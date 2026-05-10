module;

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include <QRectF>

module Doodad;

import SkinnedMesh;
import PathingTexture;
import SkeletalModelInstance;
import Terrain;
import SLK;
import Globals;
import ResourceManager;
import Utilities;

void Doodad::init(const std::string_view id, const std::shared_ptr<SkinnedMesh> mesh, const Terrain& terrain) {
	this->id = id;
	this->skin_id = id;
	this->mesh = mesh;

	mesh->reset_skeleton(skeleton);
	const bool is_doodad = doodads_slk.row_headers.contains(id);
	const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;

	pathing.reset();
	const auto path = slk.data<std::string_view>("pathtex", id);
	const auto trimmed_path = trimmed(path);
	if (!trimmed_path.empty() && trimmed_path != "none" && trimmed_path != "_") {
		const auto result = resource_manager.load<PathingTexture>(trimmed_path);
		if (result) {
			pathing = result.value();
		} else {
			std::cout << std::format("Error loading pathing texture for doodad with ID: {} ({})", id, result.error()) << '\n';
		}
	}

	update(terrain);
}

void Doodad::update(const Terrain& terrain) {
	float base_scale = 1.f;
	float max_roll;
	float max_pitch;
	if (doodads_slk.row_headers.contains(id)) {
		color.r = doodads_slk.data<float>("vertr" + std::to_string(variation + 1), id) / 255.f;
		color.g = doodads_slk.data<float>("vertg" + std::to_string(variation + 1), id) / 255.f;
		color.b = doodads_slk.data<float>("vertb" + std::to_string(variation + 1), id) / 255.f;
		max_roll = doodads_slk.data<float>("maxroll", id);
		max_pitch = doodads_slk.data<float>("maxpitch", id);
		base_scale = doodads_slk.data<float>("defscale", id);
	} else {
		color.r = destructibles_slk.data<float>("colorr", id) / 255.f;
		color.g = destructibles_slk.data<float>("colorg", id) / 255.f;
		color.b = destructibles_slk.data<float>("colorb", id) / 255.f;
		max_roll = destructibles_slk.data<float>("maxroll", id);
		max_pitch = destructibles_slk.data<float>("maxpitch", id);
	}

	glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));

	constexpr float SAMPLE_RADIUS = 32.f / 128.f;

	float pitch = 0.f;
	if (max_pitch < 0.f) {
		pitch = max_pitch;
	} else if (max_pitch > 0.f) {
		const float forward_x = position.x + (SAMPLE_RADIUS * std::cos(angle));
		const float forward_y = position.y + (SAMPLE_RADIUS * std::sin(angle));
		const float backward_x = position.x - (SAMPLE_RADIUS * std::cos(angle));
		const float backward_y = position.y - (SAMPLE_RADIUS * std::sin(angle));

		const float height1 = terrain.interpolated_height(backward_x, backward_y, false);
		const float height2 = terrain.interpolated_height(forward_x, forward_y, false);

		pitch = std::clamp(std::atan2(height2 - height1, SAMPLE_RADIUS * 2.f), -pitch, pitch);
	}
	rotation *= glm::angleAxis(-pitch, glm::vec3(0, 1, 0));

	float roll = 0.f;
	if (max_roll < 0.f) {
		roll = -max_roll;
	} else if (max_roll > 0.f) {
		const float left_of_angle = angle + (3.1415926535 / 2.0);
		const float forward_x = position.x + (SAMPLE_RADIUS * std::cos(left_of_angle));
		const float forward_y = position.y + (SAMPLE_RADIUS * std::sin(left_of_angle));
		const float backward_x = position.x - (SAMPLE_RADIUS * std::cos(left_of_angle));
		const float backward_y = position.y - (SAMPLE_RADIUS * std::sin(left_of_angle));

		const float height1 = terrain.interpolated_height(backward_x, backward_y, false);
		const float height2 = terrain.interpolated_height(forward_x, forward_y, false);

		roll = std::clamp(atan2(height2 - height1, SAMPLE_RADIUS * 2.f), -roll, roll);
	}
	rotation *= glm::angleAxis(roll, glm::vec3(1, 0, 0));

	skeleton.update_location(position, rotation, (base_scale * scale) / 128.f);
}

void SpecialDoodad::init(const std::string_view id, const std::shared_ptr<SkinnedMesh> mesh, const Terrain& terrain) {
	this->id = id;
	this->mesh = mesh;

	mesh->reset_skeleton(skeleton);

	pathing.reset();
	const auto path = doodads_slk.data<std::string_view>("pathtex", id);
	const auto trimmed_path = trimmed(path);
	if (!trimmed_path.empty() && trimmed_path != "none" && trimmed_path != "_") {
		const auto result = resource_manager.load<PathingTexture>(trimmed_path);
		if (result) {
			pathing = result.value();
		} else {
			std::cout << std::format("Error loading pathing texture for doodad with ID: {} ({})", id, result.error()) << '\n';
		}
	}

	update(terrain);
}

void SpecialDoodad::update(const Terrain& terrain) {
	position.z = terrain.interpolated_height(position.x, position.y, true);

	const float angle = doodads_slk.data<int>("fixedrot", id) / 360.f * 2.f * glm::pi<float>();
	const glm::quat rotation = glm::angleAxis(angle, glm::vec3(0, 0, 1));

	skeleton.update_location(position, rotation, glm::vec3(1.0 / 128.f));
}
