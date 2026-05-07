#pragma once

#include "w3e_loader.h"

#include <QPointF>
#include <QVector>
#include <QString>

struct WpmInfo;
struct ShadowMapInfo;

struct TerrainCornerState {
	float height = 0.0f;
	float waterHeight = 0.0f;
	quint16 rawHeight = 0;
	quint16 rawWaterAndEdge = 0;
	quint8 groundTexture = 0;
	quint8 groundVariation = 0;
	quint8 cliffTexture = 15;
	quint8 cliffVariation = 0;
	quint8 layerHeight = 2;
	bool mapEdge = false;
	bool ramp = false;
	bool blight = false;
	bool water = false;
	bool boundary = false;
};

struct TerrainTileState {
	quint8 groundTexture = 0;
	quint8 cliffTexture = 15;
	QString groundTileId;
	QString cliffTileId;
	QString groundTexturePath;
	QString cliffTexturePath;
	quint8 layerHeight = 2;
	bool ramp = false;
	bool blight = false;
	bool water = false;
	bool boundary = false;
	bool mapEdge = false;
};

struct TerrainPathingCell {
	quint8 flags = 0;
	bool canWalk = false;
	bool canFly = false;
	bool canBuild = false;
	bool isGround = false;
	bool unwalkable = false;
	bool unflyable = false;
	bool unbuildable = false;
	bool blight = false;
	bool water = false;
};

struct TerrainMeshVertex {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float u = 0.0f;
	float v = 0.0f;
	quint32 cornerIndex = 0;
};

struct TerrainMesh {
	QVector<TerrainMeshVertex> vertices;
	QVector<quint32> indices;
};

struct TerrainMaterialBatch {
	QString texturePath;
	QVector<quint32> tileIndices;
};

struct TerrainDrawBatches {
	QVector<TerrainMaterialBatch> ground;
	QVector<TerrainMaterialBatch> cliff;
	QVector<quint32> unresolvedGroundTiles;
	QVector<quint32> unresolvedCliffTiles;
};

struct TerrainModel {
	bool valid = false;
	QString error;
	QString tileset;
	qint32 version = 0;
	qint32 width = 0;
	qint32 height = 0;
	QPointF offset;
	QVector<QString> groundTileIds;
	QVector<QString> cliffTileIds;
	QVector<QString> groundTexturePaths;
	QVector<QString> cliffTexturePaths;
	QVector<TerrainCornerState> corners;
	QVector<TerrainTileState> tiles;
	qint32 pathingWidth = 0;
	qint32 pathingHeight = 0;
	QVector<TerrainPathingCell> pathing;
	QVector<bool> shadows;
	TerrainMesh mesh;
	TerrainDrawBatches drawBatches;
};

class TerrainModelBuilder {
public:
	static TerrainModel build(const W3eHeaderInfo& terrain, const WpmInfo& pathing, const ShadowMapInfo& shadowMap);
};
