#include "editor_context.h"

#include <utility>

void EditorContext::reset() {
	mapLoaded_ = false;
	loadedPath_.clear();
	map_ = MapLoadResult{};
}

void EditorContext::setLoadedMap(MapLoadResult result) {
	map_ = std::move(result);
	loadedPath_ = map_.inputPath;
	mapLoaded_ = map_.loaded;
}
