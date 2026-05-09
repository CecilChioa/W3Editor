module;

#include <QSettings>

export module Hierarchy;

import std;
import types;
import JSON;
import BinaryReader;
import CASC;
import MPQ;
import no_init_allocator;
import Utilities;

using namespace std::literals::string_literals;
namespace fs = std::filesystem;

export class Hierarchy {
  public:
	enum class ResourcePriority {
		ClassicFirst,
		ReforgedFirst,
	};

	char tileset = 'L';
	std::string locale = "enus";
	casc::CASC game_data;
	std::vector<mpq::MPQ> classic_archives;
	json::JSON aliases;

	fs::path map_directory;
	fs::path warcraft_directory;
	fs::path root_directory;

	bool ptr = false;
	bool hd = true;
	bool teen = false;
	bool local_files = true;
	bool casc_backend = false;
	ResourcePriority resource_priority = ResourcePriority::ClassicFirst;

	Hierarchy() {
		QSettings war3reg("HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III", QSettings::NativeFormat);
		local_files = war3reg.value("Allow Local Files", 0).toInt() != 0;
	}

	bool open_casc(const fs::path& directory) {
		QSettings settings;
		ptr = settings.value("flavour", "Retail").toString() == "PTR";
		hd = settings.value("hd", "False").toString() == "True";
		teen = settings.value("teen", "False").toString() == "True";
		resource_priority = settings.value("resource_priority", "ClassicFirst").toString() == "ReforgedFirst"
			? ResourcePriority::ReforgedFirst
			: ResourcePriority::ClassicFirst;

		warcraft_directory = directory;
		classic_archives.clear();
		aliases.json_data.clear();
		root_directory = warcraft_directory;
		casc_backend = false;

		if (!open_classic_archives()) {
			casc_backend = game_data.open(warcraft_directory / (ptr ? ":w3t" : ":w3"));
			root_directory = warcraft_directory / (ptr ? "_ptr_" : "_retail_");
			if (!casc_backend) {
				return false;
			}
		}

		if (auto aliases_file = open_file("filealiases.json"); aliases_file) {
			aliases.load(aliases_file.value());
		}
		return true;
	}

	bool open_classic_archives() {
		const std::array archive_names {
			"war3patch.mpq",
			"War3Patch.mpq",
			"war3xlocal.mpq",
			"War3xLocal.mpq",
			"war3x.mpq",
			"War3x.mpq",
			"war3local.mpq",
			"War3Local.mpq",
			"war3.mpq",
			"War3.mpq",
		};

		for (const auto* archive_name : archive_names) {
			const fs::path archive_path = warcraft_directory / archive_name;
			if (!fs::exists(archive_path)) {
				continue;
			}

			auto& archive = classic_archives.emplace_back();
			if (!archive.open(archive_path)) {
				classic_archives.pop_back();
			}
		}

		return !classic_archives.empty();
	}

	[[nodiscard]]
	auto open_file(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		const std::string path_str = path.string();

		for (const auto& candidate : file_candidates(path)) {
			if (auto file = open_candidate(candidate); file) {
				return file;
			}
		}

		if (aliases.exists(path_str)) {
			return open_file(aliases.alias(path_str));
		}

		return std::unexpected(path_str + " could not be found in the hierarchy");
	}

	bool file_exists(const fs::path& path) const {
		if (path.empty()) {
			return false;
		}

		for (const auto& candidate : file_candidates(path)) {
			if (candidate_exists(candidate)) {
				return true;
			}
		}

		const auto path_str = path.string();
		return aliases.exists(path_str) ? file_exists(aliases.alias(path_str)) : false;
	}

	struct FileCandidate {
		enum class Source {
			Disk,
			Map,
			GameData,
		};

		Source source;
		fs::path path;
	};

	[[nodiscard]]
	std::vector<FileCandidate> file_candidates(const fs::path& path) const {
		std::vector<FileCandidate> candidates;
		const std::string path_str = path.string();

		const auto add_disk = [&](fs::path candidate) {
			candidates.emplace_back(FileCandidate::Source::Disk, std::move(candidate));
		};
		const auto add_map = [&](fs::path candidate) {
			candidates.emplace_back(FileCandidate::Source::Map, std::move(candidate));
		};
		const auto add_game_data = [&](std::string candidate) {
			candidates.emplace_back(FileCandidate::Source::GameData, std::move(candidate));
		};
		const auto add_classic = [&]() {
			add_map(path);
			add_game_data(path_str);
			if (casc_backend) {
				add_game_data("war3.w3mod:"s + path_str);
				add_game_data(std::format("war3.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str));
				add_game_data(std::format("war3.w3mod:_locales/{}.w3mod:{}", locale, path_str));
				add_game_data("war3.w3mod:_deprecated.w3mod:"s + path_str);
				if (teen) {
					add_game_data("war3.w3mod:_teen.w3mod:"s + path_str);
				}
			}
		};
		const auto add_reforged = [&]() {
			if (hd && teen) {
				add_map("_hd.w3mod:_teen.w3mod:" + path_str);
				add_game_data("war3.w3mod:_hd.w3mod:_teen.w3mod:"s + path_str);
			}
			if (hd) {
				add_map("_hd.w3mod:" + path_str);
				add_game_data(std::format("war3.w3mod:_hd.w3mod:_tilesets/{}.w3mod:{}", tileset, path_str));
				add_game_data("war3.w3mod:_hd.w3mod:"s + path_str);
			}
		};

		add_disk("data/overrides" / path);
		if (local_files) {
			add_disk(root_directory / path);
		}

		if (resource_priority == ResourcePriority::ReforgedFirst) {
			add_reforged();
			add_classic();
		} else {
			add_classic();
			add_reforged();
		}

		return candidates;
	}

	[[nodiscard]]
	auto open_candidate(const FileCandidate& candidate) const -> std::expected<BinaryReader, std::string> {
		switch (candidate.source) {
			case FileCandidate::Source::Disk:
				return read_file(candidate.path);
			case FileCandidate::Source::Map:
				return map_file_read(candidate.path);
			case FileCandidate::Source::GameData:
				return open_game_data_file(candidate.path);
		}

		std::unreachable();
	}

	bool candidate_exists(const FileCandidate& candidate) const {
		switch (candidate.source) {
			case FileCandidate::Source::Disk:
				return fs::exists(candidate.path);
			case FileCandidate::Source::Map:
				return map_file_exists(candidate.path);
			case FileCandidate::Source::GameData:
				return game_data_file_exists(candidate.path);
		}

		std::unreachable();
	}

	[[nodiscard]]
	auto open_game_data_file(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		if (casc_backend) {
			return game_data.open_file(path.string());
		}

		for (const auto& archive : classic_archives) {
			if (!archive.file_exists(path)) {
				continue;
			}

			try {
				auto data = archive.file_open(path).read();
				std::vector<u8, default_init_allocator<u8>> buffer(data.begin(), data.end());
				return BinaryReader(std::move(buffer));
			} catch (const std::exception& error) {
				return std::unexpected(error.what());
			}
		}

		return std::unexpected(path.string() + " could not be found in the classic MPQ archives");
	}

	bool game_data_file_exists(const fs::path& path) const {
		if (casc_backend) {
			return game_data.file_exists(path.string());
		}

		return std::ranges::any_of(classic_archives, [&](const auto& archive) {
			return archive.file_exists(path);
		});
	}

	[[nodiscard]]
	auto map_file_read(const fs::path& path) const -> std::expected<BinaryReader, std::string> {
		return read_file(map_directory / path);
	}

	/// source somewhere on disk, destination relative to the map
	void map_file_add(const fs::path& source, const fs::path& destination) const {
		fs::copy_file(source, map_directory / destination, fs::copy_options::overwrite_existing);
	}

	void map_file_write(const fs::path& path, const std::vector<u8>& data) const {
		std::ofstream outfile(map_directory / path, std::ios::binary);

		if (!outfile) {
			throw std::runtime_error("Error writing file " + path.string());
		}

		outfile.write(reinterpret_cast<char const*>(data.data()), data.size());
	}

	void map_file_remove(const fs::path& path) const {
		fs::remove(map_directory / path);
	}

	bool map_file_exists(const fs::path& path) const {
		return fs::exists(map_directory / path);
	}

	void map_file_rename(const fs::path& original, const fs::path& renamed) const {
		fs::rename(map_directory / original, map_directory / renamed);
	}
};

export inline Hierarchy hierarchy;
