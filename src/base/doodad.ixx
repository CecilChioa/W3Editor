module;

#include <QRectF>

export module Doodad;

import std;
import SkinnedMesh;
import PathingTexture;
import SkeletalModelInstance;
import Terrain;
import SLK;
import Globals;
import ResourceManager;
import Utilities;
import <glm/glm.hpp>;
import <glm/gtc/quaternion.hpp>;

export struct DoodadItemSet {
	std::vector<std::pair<int, std::string>> items;
};

export struct Doodad {
	static inline int auto_increment;

	std::string id;
	std::string skin_id;
	int variation = 0;
	glm::vec3 position = glm::vec3(0.f);
	glm::vec3 scale = glm::vec3(1.f);
	float angle = 0.f;

	enum class State {
		invisible_non_solid,
		visible_non_solid,
		visible_solid
	};
	State state = State::visible_solid;
	int life = 100;

	int item_table_pointer = -1;
	std::vector<DoodadItemSet> item_sets;

	int creation_number;

	// Auxiliary data
	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	std::shared_ptr<PathingTexture> pathing;
	glm::vec3 color = glm::vec3(1.f);

	void init(const std::string_view id, const std::shared_ptr<SkinnedMesh> mesh, const Terrain& terrain);

	void update(const Terrain& terrain);

	[[nodiscard]]
	QRect get_pathing_bounding_box() const {
		if (!pathing) {
			return {};
		}

		const int rotation = glm::degrees(angle);
		const int rotated_width = rotation % 180 ? pathing->width : pathing->height;
		const int rotated_height = rotation % 180 ? pathing->height : pathing->width;
		const int x = position.x * 4 - rotated_width / 2;
		const int y = position.y * 4 - rotated_height / 2;
		return QRect {x, y, rotated_width, rotated_height};
	}

	static glm::vec2 acceptable_position(
		const glm::vec2 position,
		const std::shared_ptr<PathingTexture>& pathing,
		const float angle,
		const bool force_grid_aligned = false
	) {
		if (!pathing) {
			if (force_grid_aligned) {
				return glm::round(position * 2.f) * 0.5f;
			} else {
				return position;
			}
		}

		const int rotation = glm::degrees(angle);
		const int rotated_width = rotation % 180 ? pathing->width : pathing->height;
		const int rotated_height = rotation % 180 ? pathing->height : pathing->width;

		glm::vec2 extra_offset(0.0f);
		if (rotated_width % 4 != 0) {
			extra_offset.x = 0.25f;
		}

		if (rotated_height % 4 != 0) {
			extra_offset.y = 0.25f;
		}

		return glm::round((position + extra_offset) * 2.f) * 0.5f - extra_offset;
	}

	static float acceptable_angle(
		const std::string_view id,
		const std::shared_ptr<PathingTexture>& pathing,
		const float current_angle,
		const float target_angle
	) {
		float fixed_rotation = 0.0;
		if (doodads_slk.row_headers.contains(id)) {
			fixed_rotation = doodads_slk.data<float>("fixedrot", id);
		} else {
			fixed_rotation = destructibles_slk.data<float>("fixedrot", id);
		}

		// Negative values indicate free rotation, positive is a fixed angle
		if (fixed_rotation >= 0.0) {
			return glm::radians(fixed_rotation);
		}

		if (pathing) {
			if (pathing->width == pathing->height && pathing->homogeneous) {
				return target_angle;
			} else {
				return (static_cast<int>((target_angle + glm::pi<float>() * 0.25f) / (glm::pi<float>() * 0.5f)) % 4) * glm::pi<float>()
					* 0.5f;
			}
		} else {
			return target_angle;
		}
	}
};

export struct SpecialDoodad {
	std::string id;
	int variation;
	glm::vec3 position;
	glm::vec3 old_position;

	// Auxiliary data
	SkeletalModelInstance skeleton;
	std::shared_ptr<SkinnedMesh> mesh;
	std::shared_ptr<PathingTexture> pathing;

	void init(const std::string_view id, const std::shared_ptr<SkinnedMesh> mesh, const Terrain& terrain);

	void update(const Terrain& terrain);
};
