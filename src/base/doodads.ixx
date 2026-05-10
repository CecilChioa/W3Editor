module;

#include <QRectF>
#include <tracy/Tracy.hpp>

export module Doodads;

import std;
import std.compat;
import GLThreadPool;
import Terrain;
import Doodad;
import BinaryReader;
import BinaryWriter;
import Hierarchy;
import ResourceManager;
import SkinnedMesh;
import SkeletalModelInstance;
import PathingTexture;
import Utilities;
import Globals;
import MapInfo;
import UnorderedMap;
import SLK;
import PathingMap;
import <glm/glm.hpp>;
import <glm/gtc/matrix_transform.hpp>;

/// A class for that contains doodad and takes care of loading/saving war3map.doo files
export class Doodads {
	// TODO: this shouldn't be holding these (forever) as the resources will never be released, NOR does it clear cache right now due to object editor usage
	hive::unordered_map<std::string, std::shared_ptr<SkinnedMesh>> id_to_mesh;
	std::mutex mesh_mutex;

	static constexpr int write_version = 8;
	static constexpr int write_subversion = 11;
	static constexpr int write_special_version = 0;

  public:
	std::vector<SpecialDoodad> special_doodads;
	std::vector<Doodad> doodads;

	bool load(const Terrain& terrain, const MapInfo& info);

	void save(const Terrain& terrain) const;

	void create(Terrain& terrain, PathingMap& pathing_map);

	// Will assign a creation number
	Doodad& add_doodad(std::string id, const int variation, const glm::vec3 position, const Terrain& terrain);

	// You will have to manually set a creation number and valid skin ID
	Doodad& add_doodad(const Doodad& doodad);

	void remove_doodad(Doodad* doodad);

	void remove_special_doodad(SpecialDoodad* doodad);

	void remove_doodads(const std::unordered_set<Doodad*>& list);

	void remove_special_doodads(const std::unordered_set<SpecialDoodad*>& list);

	std::vector<Doodad*> query_area(const QRectF& area);

	/// Returns an AABB in pathing map tiles (4x4 per whole tile) surrounding the doodads accounting for their pathing maps
	QRect get_pathing_bounding_box(const std::vector<Doodad>& doodads) const;
	QRect get_pathing_bounding_box(const std::vector<Doodad*>& doodads) const;

	void update_doodad_pathing(const std::vector<Doodad>& doodads, PathingMap& pathing_map);
	void update_doodad_pathing(const std::vector<Doodad*>& doodads, PathingMap& pathing_map);

	/// The input area should be in pathing map tiles.
	void update_doodad_pathing(const QRect& area, PathingMap& pathing_map);

	/// The input area should be in whole tile fractions (of 128 WC3 units).
	void update_special_doodad_pathing(const QRectF& area, Terrain& terrain) const;

	void process_doodad_field_change(const std::string& id, const std::string& field, const Terrain& terrain);

	void process_destructible_field_change(const std::string& id, const std::string& field, const Terrain& terrain);

	std::shared_ptr<SkinnedMesh> get_mesh(std::string id, int variation);
};
