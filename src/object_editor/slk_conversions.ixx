module;

#include <random>
#include <string>
#include <string_view>

export module SlkConversions;

import Map;
import MapGlobal;
import SLK;
import Globals;

/// Returns the new destructible ID
export std::string convert_doodad_to_destructible(const std::string_view doodad_id, const std::string_view base_destructible) {
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_int_distribution<int> dist(0, 25);

	std::string new_id;
	for (;;) {
		new_id.clear();
		new_id.push_back(static_cast<char>('A' + dist(mt)));
		new_id.push_back(static_cast<char>('a' + dist(mt)));
		new_id.push_back(static_cast<char>('a' + dist(mt)));
		new_id.push_back(static_cast<char>('a' + dist(mt)));

		if (!units_slk.row_headers.contains(new_id) && !items_slk.row_headers.contains(new_id) && !abilities_slk.row_headers.contains(new_id)
			&& !doodads_slk.row_headers.contains(new_id) && !destructibles_slk.row_headers.contains(new_id)
			&& !upgrade_slk.row_headers.contains(new_id) && !buff_slk.row_headers.contains(new_id)) {
			break;
		}
	}

	destructibles_slk.copy_row(base_destructible, new_id, false);

	const auto copy_field = [&](const std::string_view header_from, const std::string_view header_to) {
		const auto data = doodads_slk.data(std::string(header_from), doodad_id);
		destructibles_slk.set_shadow_data(std::string(header_to), new_id, data);
	};

	copy_field("fixedrot", "fixedrot");
	copy_field("maxroll", "maxroll");
	copy_field("maxpitch", "maxpitch");
	copy_field("showinmm", "showinmm");
	copy_field("usemmcolor", "usemmcolor");
	copy_field("mmred", "mmred");
	copy_field("mmgreen", "mmgreen");
	copy_field("mmblue", "mmblue");
	copy_field("file", "file");
	copy_field("selsize", "selsize");
	copy_field("showinfog", "fogvis");
	copy_field("vertr1", "colorr");
	copy_field("vertg1", "colorg");
	copy_field("vertb1", "colorb");
	copy_field("numvar", "numvar");
	copy_field("canscalerandscale", "canscalerandscale");
	copy_field("tilesetspecific", "tilesetspecific");
	copy_field("maxscale", "maxscale");
	copy_field("minscale", "minscale");
	copy_field("userlist", "userlist");
	copy_field("oncliffs", "oncliffs");
	copy_field("onwater", "onwater");
	copy_field("tilesets", "tilesets");
	copy_field("useclickhelper", "useclickhelper");
	copy_field("pathtex", "pathtex");
	copy_field("walkable", "walkable");
	copy_field("soundloop", "loopsound");
	copy_field("name", "name");
	return new_id;
}
