module;

#include <vector>
#include <filesystem>
#include <expected>
#include <string>
#include <utility>

export module Texture;

import Hierarchy;
import BLP;
import BinaryReader;
import ResourceManager;
import <soil2/SOIL2.h>;

namespace fs = std::filesystem;

export class Texture : public Resource {
  public:
	int width;
	int height;
	int channels;
	std::vector<uint8_t> data;

	static constexpr const char* name = "Texture";

	static std::vector<fs::path> classic_texture_candidates(const fs::path& path) {
		std::vector<fs::path> candidates;
		fs::path candidate = path;

		candidate.replace_extension(".blp");
		candidates.push_back(candidate);
		candidate.replace_extension(".tga");
		candidates.push_back(candidate);
		candidate.replace_extension(".dds");
		candidates.push_back(candidate);

		return candidates;
	}

	static std::vector<fs::path> reforged_texture_candidates(const fs::path& path) {
		std::vector<fs::path> candidates;
		fs::path candidate = path;
		candidate.replace_filename(path.stem().string() + "_diffuse");

		candidate.replace_extension(".dds");
		candidates.push_back(candidate);
		candidate.replace_extension(".tga");
		candidates.push_back(candidate);
		candidate.replace_extension(".blp");
		candidates.push_back(candidate);

		return candidates;
	}

	static std::vector<fs::path> texture_candidates(const fs::path& path) {
		std::vector<fs::path> candidates;
		const auto classic = classic_texture_candidates(path);
		const auto reforged = reforged_texture_candidates(path);

		const auto append = [&](const std::vector<fs::path>& paths) {
			candidates.insert(candidates.end(), paths.begin(), paths.end());
		};

		if (hierarchy.resource_priority == Hierarchy::ResourcePriority::ReforgedFirst) {
			append(reforged);
			append(classic);
		} else {
			append(classic);
			append(reforged);
		}

		return candidates;
	}

	explicit Texture() = default;
	explicit Texture(const fs::path& path) {
		fs::path loaded_path;
		std::expected<BinaryReader, std::string> loaded = std::unexpected(path.string() + " could not be found in the hierarchy");

		for (const auto& candidate : texture_candidates(path)) {
			loaded_path = candidate;
			loaded = hierarchy.open_file(candidate);
			if (loaded) {
				break;
			}
		}

		BinaryReader reader = std::move(loaded)
			.or_else([&](const std::string&) {
				std::cout << std::format("Error loading texture {}", loaded_path.string()) << '\n';
				loaded_path = "Textures/btntempw.dds";
				return hierarchy.open_file(loaded_path);
			})
			.value();

		uint8_t* image_data;

		if (loaded_path.extension() == ".blp") {
			image_data = blp::load(reader, width, height, channels);
		} else {
			image_data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		}
		data = std::vector<uint8_t>(image_data, image_data + width * height * channels);
		delete image_data;
	}
};