module;

#include <glad/glad.h>

export module RenderManager;

import std;
import types;
import SkinnedMesh;
import Shader;
import SkeletalModelInstance;
import ResourceManager;
import Timer;
import MDX;
import Camera;
import Utilities;
import Globals;
import Units;
import SLK;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;
import <glm/gtc/quaternion.hpp>;
import Doodads;

export class RenderManager {
  public:
	struct SkinnedInstance {
		SkinnedMesh* mesh;
		uint32_t instance_id;
		float distance;
	};

	std::shared_ptr<Shader> skinned_mesh_shader_sd;
	std::shared_ptr<Shader> skinned_mesh_shader_hd;
	std::shared_ptr<Shader> colored_skinned_shader;

	std::vector<SkinnedMesh*> skinned_meshes;
	std::vector<SkinnedInstance> skinned_transparent_instances;

	std::shared_ptr<SkinnedMesh> click_helper;
	std::vector<SkeletalModelInstance> click_helper_instances;

	GLuint color_buffer;
	GLuint depth_buffer;
	GLuint color_picking_framebuffer;

	int window_width;
	int window_height;

	RenderManager();
	~RenderManager();

	void queue_render(SkinnedMesh& skinned_mesh, const SkeletalModelInstance& skeleton, glm::vec3 color, uint32_t team_color_index);
	void queue_click_helper(const glm::mat4& model);
	void render(bool render_lighting, glm::vec3 light_direction);
	void resize_framebuffers(int width, int height);

	[[nodiscard]]
	std::optional<size_t> pick_unit_id_under_mouse(const Units& units, glm::vec2 mouse_position) const;

	[[nodiscard]]
	std::optional<size_t> pick_doodad_id_under_mouse(const Doodads& doodads, glm::vec2 mouse_position) const;
};
