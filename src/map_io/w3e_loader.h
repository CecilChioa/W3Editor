#pragma once

#include <QByteArray>
#include <QPointF>
#include <QString>
#include <QVector>

struct W3eCorner {
	bool mapEdge = false;
	quint8 groundTexture = 0;
	float height = 0.0f;
	float waterHeight = 0.0f;
	bool ramp = false;
	bool blight = false;
	bool water = false;
	bool boundary = false;
	quint8 groundVariation = 0;
	quint8 cliffVariation = 0;
	quint8 cliffTexture = 15;
	quint8 layerHeight = 2;
	quint16 rawHeight = 0;
	quint16 rawWaterAndEdge = 0;
};

struct W3eHeaderInfo {
	bool valid = false;
	QString sourcePath;
	QString tileset;
	qint32 version = 0;
	qint32 customTileset = 0;
	qint32 groundTileCount = 0;
	qint32 cliffTileCount = 0;
	qint32 width = 0;
	qint32 height = 0;
	QPointF offset;
	QVector<QString> groundTileIds;
	QVector<QString> cliffTileIds;
	QVector<W3eCorner> corners;
	float minCornerHeight = 0.0f;
	float maxCornerHeight = 0.0f;
	float minWaterHeight = 0.0f;
	float maxWaterHeight = 0.0f;
	qint32 waterCornerCount = 0;
	qint32 boundaryCornerCount = 0;
	qint32 rampCornerCount = 0;
	qint32 blightCornerCount = 0;
	qint32 mapEdgeCornerCount = 0;
	QString error;
};

class W3eLoader {
public:
	static W3eHeaderInfo load(const QString& path);
	static W3eHeaderInfo parseW3eFromBytes(const QByteArray& bytes, const QString& sourcePath);
};
