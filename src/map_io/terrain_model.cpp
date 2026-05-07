#include "terrain_model.h"

#include "map_loader.h"

#include <algorithm>

namespace {
int cornerIndex(const TerrainModel& model, int x, int y) {
	return y * model.width + x;
}

const TerrainCornerState& cornerAt(const TerrainModel& model, int x, int y) {
	return model.corners[cornerIndex(model, x, y)];
}

TerrainTileState buildTile(const TerrainModel& model, int x, int y) {
	const TerrainCornerState& c00 = cornerAt(model, x, y);
	const TerrainCornerState& c10 = cornerAt(model, x + 1, y);
	const TerrainCornerState& c01 = cornerAt(model, x, y + 1);
	const TerrainCornerState& c11 = cornerAt(model, x + 1, y + 1);

	TerrainTileState tile;
	tile.groundTexture = c00.groundTexture;
	tile.cliffTexture = c00.cliffTexture;
	tile.layerHeight = c00.layerHeight;
	tile.ramp = c00.ramp || c10.ramp || c01.ramp || c11.ramp;
	tile.blight = c00.blight || c10.blight || c01.blight || c11.blight;
	tile.water = c00.water || c10.water || c01.water || c11.water;
	tile.boundary = c00.boundary || c10.boundary || c01.boundary || c11.boundary;
	tile.mapEdge = c00.mapEdge || c10.mapEdge || c01.mapEdge || c11.mapEdge;
	return tile;
}

TerrainMeshVertex vertexForCorner(const TerrainModel& model, int x, int y) {
	const TerrainCornerState& corner = cornerAt(model, x, y);
	TerrainMeshVertex vertex;
	vertex.x = static_cast<float>(model.offset.x() + x * 128.0);
	vertex.y = static_cast<float>(model.offset.y() + y * 128.0);
	vertex.z = corner.height;
	vertex.u = static_cast<float>(x);
	vertex.v = static_cast<float>(y);
	vertex.cornerIndex = static_cast<quint32>(cornerIndex(model, x, y));
	return vertex;
}

void buildMesh(TerrainModel* model) {
	model->mesh.vertices.reserve(model->width * model->height);
	for (int y = 0; y < model->height; ++y) {
		for (int x = 0; x < model->width; ++x) {
			model->mesh.vertices.push_back(vertexForCorner(*model, x, y));
		}
	}

	const int tileWidth = std::max(0, model->width - 1);
	const int tileHeight = std::max(0, model->height - 1);
	model->mesh.indices.reserve(tileWidth * tileHeight * 6);
	for (int y = 0; y < tileHeight; ++y) {
		for (int x = 0; x < tileWidth; ++x) {
			const quint32 i00 = static_cast<quint32>(cornerIndex(*model, x, y));
			const quint32 i10 = static_cast<quint32>(cornerIndex(*model, x + 1, y));
			const quint32 i01 = static_cast<quint32>(cornerIndex(*model, x, y + 1));
			const quint32 i11 = static_cast<quint32>(cornerIndex(*model, x + 1, y + 1));
			model->mesh.indices << i00 << i10 << i11;
			model->mesh.indices << i00 << i11 << i01;
		}
	}
}
}

TerrainModel TerrainModelBuilder::build(const W3eHeaderInfo& terrain, const WpmInfo& pathing, const ShadowMapInfo& shadowMap) {
	TerrainModel model;
	if (!terrain.valid) {
		model.error = terrain.error.isEmpty() ? QStringLiteral("terrain is invalid") : terrain.error;
		return model;
	}
	if (terrain.width <= 0 || terrain.height <= 0 || terrain.corners.size() != terrain.width * terrain.height) {
		model.error = QStringLiteral("terrain corner dimensions are inconsistent");
		return model;
	}

	model.valid = true;
	model.tileset = terrain.tileset;
	model.version = terrain.version;
	model.width = terrain.width;
	model.height = terrain.height;
	model.offset = terrain.offset;
	model.groundTileIds = terrain.groundTileIds;
	model.cliffTileIds = terrain.cliffTileIds;
	model.corners.reserve(terrain.corners.size());

	for (const W3eCorner& source : terrain.corners) {
		TerrainCornerState corner;
		corner.height = source.height;
		corner.waterHeight = source.waterHeight;
		corner.rawHeight = source.rawHeight;
		corner.rawWaterAndEdge = source.rawWaterAndEdge;
		corner.groundTexture = source.groundTexture;
		corner.groundVariation = source.groundVariation;
		corner.cliffTexture = source.cliffTexture;
		corner.cliffVariation = source.cliffVariation;
		corner.layerHeight = source.layerHeight;
		corner.mapEdge = source.mapEdge;
		corner.ramp = source.ramp;
		corner.blight = source.blight;
		corner.water = source.water;
		corner.boundary = source.boundary;
		model.corners.push_back(corner);
	}

	const int tileWidth = std::max(0, model.width - 1);
	const int tileHeight = std::max(0, model.height - 1);
	model.tiles.reserve(tileWidth * tileHeight);
	for (int y = 0; y < tileHeight; ++y) {
		for (int x = 0; x < tileWidth; ++x) {
			model.tiles.push_back(buildTile(model, x, y));
		}
	}

	if (pathing.valid && pathing.flags.size() == pathing.width * pathing.height) {
		model.pathingWidth = static_cast<qint32>(pathing.width);
		model.pathingHeight = static_cast<qint32>(pathing.height);
		model.pathing.reserve(pathing.flags.size());
		for (quint8 flags : pathing.flags) {
			TerrainPathingCell cell;
			cell.flags = flags;
			cell.canWalk = (flags & 0x02) != 0;
			cell.canFly = (flags & 0x04) != 0;
			cell.canBuild = (flags & 0x08) != 0;
			cell.isGround = (flags & 0x40) != 0;
			cell.unwalkable = !cell.canWalk;
			cell.unflyable = !cell.canFly;
			cell.unbuildable = !cell.canBuild;
			cell.blight = (flags & 0x20) != 0;
			cell.water = !cell.isGround;
			model.pathing.push_back(cell);
		}
	}

	if (shadowMap.valid) {
		model.shadows = shadowMap.shadows;
	}

	buildMesh(&model);
	return model;
}
