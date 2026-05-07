#include "map_load_report_formatter.h"

namespace {
QString joinStrings(const QVector<QString>& values) {
	QStringList list;
	list.reserve(values.size());
	for (const QString& value : values) list.push_back(value);
	return list.join(QStringLiteral(", "));
}
}

MapLoadReport MapLoadReportFormatter::format(const QString& inputPath, const MapLoadResult& result) {
	MapLoadReport report;
	report.lines.push_back(QStringLiteral("--------"));
	report.lines.push_back(QStringLiteral("Input: %1").arg(inputPath));
	for (const QString& line : result.log) report.lines.push_back(line);

	if (!result.loaded) {
		report.success = false;
		report.failureLine = QStringLiteral("Read failed: %1").arg(result.error.isEmpty() ? result.terrain.error : result.error);
		return report;
	}

	report.success = true;
	report.lines.push_back(QStringLiteral("Read success"));

	const auto& info = result.terrain;
	report.lines.push_back(QStringLiteral("Source: %1").arg(info.sourcePath));
	report.lines.push_back(QStringLiteral("W3E version: %1").arg(info.version));
	report.lines.push_back(QStringLiteral("Tileset: %1").arg(info.tileset));
	report.lines.push_back(QStringLiteral("CustomTileset: %1").arg(info.customTileset));
	report.lines.push_back(QStringLiteral("GroundTiles: %1").arg(info.groundTileCount));
	report.lines.push_back(QStringLiteral("GroundTileIds: %1").arg(joinStrings(info.groundTileIds)));
	report.lines.push_back(QStringLiteral("CliffTiles: %1").arg(info.cliffTileCount));
	report.lines.push_back(QStringLiteral("CliffTileIds: %1").arg(joinStrings(info.cliffTileIds)));
	report.lines.push_back(QStringLiteral("Size: %1 x %2").arg(info.width).arg(info.height));
	report.lines.push_back(QStringLiteral("Offset: %1, %2").arg(info.offset.x()).arg(info.offset.y()));
	report.lines.push_back(QStringLiteral("Corners: %1").arg(info.corners.size()));
	report.lines.push_back(QStringLiteral("Height range: %1 .. %2").arg(info.minCornerHeight).arg(info.maxCornerHeight));
	report.lines.push_back(QStringLiteral("Water height range: %1 .. %2").arg(info.minWaterHeight).arg(info.maxWaterHeight));
	report.lines.push_back(QStringLiteral("Flags: water=%1 boundary=%2 ramp=%3 blight=%4 edge=%5")
		.arg(info.waterCornerCount)
		.arg(info.boundaryCornerCount)
		.arg(info.rampCornerCount)
		.arg(info.blightCornerCount)
		.arg(info.mapEdgeCornerCount));

	return report;
}
