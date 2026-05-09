export module GroundTexture;

import std;
import BinaryReader;
import ResourceManager;
import OpenGLUtilities;
import BLP;
import Hierarchy;
import <soil2/SOIL2.h>;
import <glm/glm.hpp>;
import <glad/glad.h>;

namespace fs = std::filesystem;

export class GroundTexture : public Resource {
  public:
	GLuint id = 0;
	GLuint64 bindless_handle = 0;
	int tile_size;
	bool extended = false;
	glm::vec4 minimap_color;

	static constexpr const char* name = "GroundTexture";

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

	static auto load_texture_reader(const fs::path& path, fs::path& loaded_path) -> std::expected<BinaryReader, std::string> {
		std::string last_error;

		for (const auto& candidate : texture_candidates(path)) {
			loaded_path = candidate;
			if (auto reader = hierarchy.open_file(candidate); reader) {
				return reader;
			} else {
				last_error = reader.error();
			}
		}

		return std::unexpected(last_error.empty() ? path.string() + " could not be found in the hierarchy" : last_error);
	}

	explicit GroundTexture(const fs::path& path) {
		fs::path loaded_path;
		BinaryReader reader = load_texture_reader(path, loaded_path)
			.or_else([&](const std::string&) {
				std::println("Error loading texture {}", loaded_path.string());
				loaded_path = "Textures/btntempw.dds";
				return hierarchy.open_file(loaded_path);
			})
			.value();

		int width;
		int height;
		int channels;
		uint8_t* data;

		const auto extension = loaded_path.extension().string();
		const bool is_blp = extension == ".blp" || extension == ".BLP";
		if (is_blp) {
			data = blp::load(reader, width, height, channels);
		} else {
			data = SOIL_load_image_from_memory(reader.buffer.data(), static_cast<int>(reader.buffer.size()), &width, &height, &channels, SOIL_LOAD_AUTO);
		}

		tile_size = std::max(height * 0.25f, 1.f);
		extended = (width == height * 2);
		int lods = log2(tile_size) + 1;

		const int format = channels == 3 ? GL_RGB : GL_RGBA;
		const int bit_format = channels == 3 ? GL_RGB8 : GL_RGBA8;

		glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &id);
		glTextureStorage3D(id, lods, bit_format, tile_size, tile_size, extended ? 32 : 16);
		glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				glTextureSubImage3D(id, 0, 0, 0, y * 4 + x, tile_size, tile_size, 1, format, GL_UNSIGNED_BYTE, data + (y * tile_size * width + x * tile_size) * channels);

				if (extended) {
					glTextureSubImage3D(id, 0, 0, 0, y * 4 + x + 16, tile_size, tile_size, 1, format, GL_UNSIGNED_BYTE, data + (y * tile_size * width + (x + 4) * tile_size) * channels);
				}
			}
		}
		if (is_blp) {
			delete[] data;
		} else {
			delete data;
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glGenerateTextureMipmap(id);

		glGetTextureSubImage(id, lods - 1, 0, 0, 0, 1, 1, 1, format, GL_FLOAT, 16, &minimap_color);
		minimap_color *= 255.f;

		bindless_handle = glGetTextureHandleARB(id);
		glMakeTextureHandleResidentARB(bindless_handle);
	}

	virtual ~GroundTexture() {
		glMakeTextureHandleNonResidentARB(bindless_handle);
		glDeleteTextures(1, &id);
	}
};