#pragma once

#include "../map_io/map_loader.h"

#include <QString>

class EditorContext {
public:
	void reset();
	void setLoadedMap(MapLoadResult result);

	bool hasMapLoaded() const { return mapLoaded_; }
	const QString& loadedPath() const { return loadedPath_; }
	const MapLoadResult& map() const { return map_; }

private:
	bool mapLoaded_ = false;
	QString loadedPath_;
	MapLoadResult map_;
};
