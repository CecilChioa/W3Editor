module;

#include <cstdint>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include <glad/glad.h>

module RenderManager;

import SkinnedMesh;
import Shader;
import SkeletalModelInstance;
import ResourceManager;
import MDX;
import Camera;
import Utilities;
import Globals;
import Units;
import SLK;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import Doodads;

RenderManager::RenderManager() {
	skinned_mesh_shader_sd = resource_manager.load<Shader>({"data/shaders/skinned_mesh_sd.vert", "data/shaders/skinned_mesh_sd.frag"}).value();
	skinned_mesh_shader_hd = resource_manager.load<Shader>({"data/shaders/skinned_mesh_hd.vert", "data/shaders/skinned_mesh_hd.frag"}).value();
	colored_skinned_shader = resource_manager.load<Shader>(
		{"data/shaders/skinned_mesh_instance_color_coded.vert", "data/shaders/skinned_mesh_instance_color_coded.frag"}
	).value();

	click_helper = resource_manager.load<SkinnedMesh>("Objects/InvalidObject/InvalidObject.mdx", "", std::nullopt).value();

	glCreateFramebuffers(1, &color_picking_framebuffer);

	glCreateRenderbuffers(1, &color_buffer);
	glNamedRenderbufferStorage(color_buffer, GL_RGBA8, 800, 600);
	glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buffer);

	glCreateRenderbuffers(1, &depth_buffer);
	glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, 800, 600);
	glNamedFramebufferRenderbuffer(color_picking_framebuffer, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);

	if (glCheckNamedFramebufferStatus(color_picking_framebuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << std::format("ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n") << '\n';
	}
}

RenderManager::~RenderManager() {
	glDeleteRenderbuffers(1, &color_buffer);
	glDeleteRenderbuffers(1, &depth_buffer);
	glDeleteFramebuffers(1, &color_picking_framebuffer);
}

void RenderManager::queue_render(
	SkinnedMesh& skinned_mesh,
	const SkeletalModelInstance& skeleton,
	const glm::vec3 color,
	const uint32_t team_color_index
) {
	const mdx::Extent& extent = skinned_mesh.mdx->sequences[skeleton.sequence_index].extent;

	if (!camera.inside_frustrum_transform(extent.minimum, extent.maximum, skeleton.matrix)) {
		return;
	}

	skinned_mesh.render_jobs.push_back(skeleton.matrix);
	skinned_mesh.render_colors.push_back(color);
	skinned_mesh.render_team_color_indexes.push_back(team_color_index);
	skinned_mesh.skeletons.push_back(&skeleton);

	if (skinned_mesh.render_jobs.size() == 1) {
		skinned_meshes.push_back(&skinned_mesh);
	}

	if (!skinned_mesh.has_mesh) {
		return;
	}

	if (skinned_mesh.has_transparent_layers) {
		skinned_transparent_instances.push_back(
			RenderManager::SkinnedInstance {
				.mesh = &skinned_mesh,
				.instance_id = static_cast<uint32_t>(skinned_mesh.render_jobs.size() - 1),
				.distance = glm::distance(camera.position - camera.direction * camera.distance, glm::vec3(skeleton.matrix[3])),
			}
		);
	}
}

void RenderManager::queue_click_helper(const glm::mat4& model) {
	auto a = SkeletalModelInstance(click_helper->mdx);
	a.matrix = model;
	a.update(0.016f);
	click_helper_instances.push_back(a);
}

void RenderManager::render(const bool render_lighting, const glm::vec3 light_direction) {
	GLint old_vao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

	for (const auto& i : click_helper_instances) {
		queue_render(*click_helper, i, glm::vec3(1.f), 0);
	}

	for (const auto& i : skinned_meshes) {
		i->upload_render_data();
	}

	skinned_mesh_shader_sd->use();
	glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
	glUniform3fv(3, 1, &light_direction.x);
	glBlendFunc(GL_ONE, GL_ZERO);

	for (const auto& i : skinned_meshes) {
		i->render_opaque(false, render_lighting);
	}

	skinned_mesh_shader_hd->use();
	glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
	glUniform3fv(3, 1, &light_direction.x);

	for (const auto& i : skinned_meshes) {
		i->render_opaque(true, render_lighting);
	}

	std::ranges::sort(skinned_transparent_instances, [](auto& left, auto& right) {
		return left.distance > right.distance;
	});
	glEnable(GL_BLEND);
	glDepthMask(false);

	skinned_mesh_shader_sd->use();
	glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
	glUniform3fv(3, 1, &light_direction.x);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent(i.instance_id, false, render_lighting);
	}

	skinned_mesh_shader_hd->use();
	glUniformMatrix4fv(0, 1, false, &camera.projection_view[0][0]);
	glUniform3fv(3, 1, &light_direction.x);

	for (const auto& i : skinned_transparent_instances) {
		i.mesh->render_transparent(i.instance_id, true, render_lighting);
	}

	glBindVertexArray(old_vao);
	glDepthMask(true);

	for (const auto& i : skinned_meshes) {
		i->clear_render_data();
	}

	click_helper_instances.clear();
	skinned_meshes.clear();
	skinned_transparent_instances.clear();
}

void RenderManager::resize_framebuffers(const int width, const int height) {
	glNamedRenderbufferStorage(color_buffer, GL_RGBA8, width, height);
	glNamedRenderbufferStorage(depth_buffer, GL_DEPTH24_STENCIL8, width, height);
	window_width = width;
	window_height = height;
}

std::optional<size_t> RenderManager::pick_unit_id_under_mouse(const Units& units, const glm::vec2 mouse_position) const {
	GLint old_fbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
	GLint old_vao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

	glBindFramebuffer(GL_FRAMEBUFFER, color_picking_framebuffer);

	glClearColor(0, 0, 0, 1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, window_width, window_height);

	glDepthMask(true);
	glDisable(GL_BLEND);

	colored_skinned_shader->use();
	for (size_t i = 0; i < units.units.size(); i++) {
		const Unit& unit = units.units[i];
		if (unit.id == "sloc") {
			continue;
		}

		const mdx::Extent& extent = unit.mesh->mdx->sequences[unit.skeleton.sequence_index].extent;
		if (camera.inside_frustrum_transform(extent.minimum, extent.maximum, unit.skeleton.matrix)) {
			unit.mesh->render_color_coded(unit.skeleton, i + 1);
		}
	}

	glm::u8vec4 color;
	glReadPixels(mouse_position.x, window_height - mouse_position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

	glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	glBindVertexArray(old_vao);
	glEnable(GL_BLEND);

	const int index = color.r + (color.g << 8) + (color.b << 16);
	if (index != 0) {
		return {index - 1};
	} else {
		return {};
	}
}

std::optional<size_t> RenderManager::pick_doodad_id_under_mouse(const Doodads& doodads, const glm::vec2 mouse_position) const {
	GLint old_fbo;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_fbo);
	GLint old_vao;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);

	glBindFramebuffer(GL_FRAMEBUFFER, color_picking_framebuffer);

	glClearColor(0, 0, 0, 1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, window_width, window_height);

	glDepthMask(true);
	glDisable(GL_BLEND);

	glm::vec3 window = {input_handler.mouse.x, window_height - input_handler.mouse.y, 1.f};
	glm::vec3 pos = glm::unProject(window, camera.view, camera.projection, glm::vec4(0, 0, window_width, window_height));
	glm::vec3 ray_origin = camera.position - camera.direction * camera.distance;
	glm::vec3 ray_direction = glm::normalize(pos - ray_origin);

	colored_skinned_shader->use();
	for (size_t i = 0; i < doodads.doodads.size(); i++) {
		const Doodad& doodad = doodads.doodads[i];

		const mdx::Extent& extent = doodad.mesh->mdx->sequences[doodad.skeleton.sequence_index].extent;
		glm::vec3 local_min = extent.minimum;
		glm::vec3 local_max = extent.maximum;

		const bool is_doodad = doodads_slk.row_headers.contains(doodad.id);
		const slk::SLK& slk = is_doodad ? doodads_slk : destructibles_slk;
		bool use_click_helper = slk.data<bool>("useclickhelper", doodad.id);
		if (use_click_helper) {
			local_min = glm::min(local_min, click_helper->mdx->extent.minimum);
			local_max = glm::max(local_max, click_helper->mdx->extent.maximum);
		}

		glm::vec3 min;
		glm::vec3 max;
		transform_aabb_non_uniform(local_min, local_max, min, max, doodad.skeleton.matrix);

		if (intersect_aabb(min, max, ray_origin, ray_direction)) {
			doodad.mesh->render_color_coded(doodad.skeleton, i + 1);

			if (use_click_helper) {
				auto a = SkeletalModelInstance(click_helper->mdx);
				a.matrix = doodad.skeleton.matrix;
				a.update(0.016f);
				click_helper->render_color_coded(a, i + 1);
			}
		}
	}

	glm::u8vec4 color;
	glReadPixels(mouse_position.x, window_height - mouse_position.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &color);

	glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	glBindVertexArray(old_vao);
	glEnable(GL_BLEND);

	const int index = color.r + (color.g << 8) + (color.b << 16);
	if (index != 0) {
		return {index - 1};
	} else {
		return {};
	}
}