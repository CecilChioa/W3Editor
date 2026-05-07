#pragma once

#include "../map_io/map_loader.h"

#include <QStringList>

struct MapLoadReport {
	QStringList lines;
	QString failureLine;
	bool success = false;
};

class MapLoadReportFormatter {
public:
	static MapLoadReport format(const QString& inputPath, const MapLoadResult& result);
};
