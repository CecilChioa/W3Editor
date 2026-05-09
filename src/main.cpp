#define MI_MALLOC_OVERRIDE
#include <mimalloc.h>

#define QT_NO_OPENGL

#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QFont>
#include <QPalette>
#include <QSurfaceFormat>
#include <QSettings>
#include <QStyleFactory>

#include "main_window/hivewe.h"
#include "DockManager.h"

#ifdef WIN32
// To force HiveWE to run on the discrete GPU if available
extern "C" {
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
}
#endif

#include <tracy/Tracy.hpp>

import std;
import Map;
import Timer;
import MapGlobal;
import Globals;
import Utilities;
import Hierarchy;
import BinaryReader;
import GLThreadPool;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
	std::ofstream startup_log("hivewe-startup-trace.log", std::ios::app);
	const auto trace = [&](std::string_view message) {
		startup_log << message << '\n';
		startup_log.flush();
	};
	trace("main entered");
	ZoneScopedN("main");

	Timer start_timer;
	trace("timer created");

	QSurfaceFormat format;
	format.setDepthBufferSize(24);
	format.setStencilBufferSize(8);
	format.setVersion(4, 5);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setOption(QSurfaceFormat::DebugContext);
	format.setSwapInterval(1);
	//format.setColorSpace(QSurfaceFormat::sRGBColorSpace);
	QSurfaceFormat::setDefaultFormat(format);
	trace("surface format set");

	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	QCoreApplication::setOrganizationName("HiveWE");
	QCoreApplication::setApplicationName("HiveWE");
	trace("core application attributes set");

	QLocale::setDefault(QLocale("en_US"));
	trace("locale set");

	// Create a dark palette
	// For some magically unknown reason Qt draws Qt::white text as black, so we use QColor(255, 254, 255) instead
	QPalette darkPalette;
	darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::WindowText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::Base, QColor(42, 42, 42));
	darkPalette.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
	darkPalette.setColor(QPalette::ToolTipBase, QColor(66, 66, 66));
	darkPalette.setColor(QPalette::ToolTipText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Text, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::PlaceholderText, Qt::gray);
	darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::Dark, QColor(35, 35, 35));
	darkPalette.setColor(QPalette::Shadow, QColor(20, 20, 20));
	darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
	darkPalette.setColor(QPalette::ButtonText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
	darkPalette.setColor(QPalette::BrightText, Qt::red);
	darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
	darkPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
	darkPalette.setColor(QPalette::HighlightedText, QColor(255, 254, 255));
	darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));

	QApplication::setPalette(darkPalette);
	QApplication::setStyle("Fusion");
	trace("application static style set");

	QApplication a(argc, argv);
	trace("qapplication created");

	ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting);
	ads::CDockManager::setConfigFlag(ads::CDockManager::AllTabsHaveCloseButton);
	ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility);
	ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize);
	ads::CDockManager::setConfigFlag(ads::CDockManager::MiddleMouseButtonClosesTab);
	trace("dock manager configured");

	QSettings settings;
	QFile file("data/themes/" + settings.value("theme", "Dark").toString() + ".qss");
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Error: Reading theme failed:" << file.error() << ": " << file.errorString();
		return -1;
	}

	a.setStyleSheet(QLatin1String(file.readAll()));
	trace("stylesheet applied");

	fs::path warcraft_directory = find_warcraft_directory();
	trace(std::format("trying warcraft directory: {}", warcraft_directory.string()));
	while (!hierarchy.open_casc(warcraft_directory)) {
		trace(std::format("open_casc failed: {}", warcraft_directory.string()));
		warcraft_directory = QFileDialog::getExistingDirectory(nullptr, "Select Warcraft Directory", "/home", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks).toStdWString();
		trace(std::format("trying warcraft directory: {}", warcraft_directory.string()));
		if (warcraft_directory == "") {
			return EXIT_SUCCESS;
		}
	}
	trace("open_casc succeeded");
	settings.setValue("warcraftDirectory", QString::fromStdString(warcraft_directory.string()));

	const auto casc_future = std::async(std::launch::async, [&]() {
		trace("casc thread entered");

		// Place common.j and blizzard.j in the data folder. Required by JassHelper
		BinaryReader common = hierarchy.open_file("scripts/common.j").value();
		std::ofstream output("data/tools/common.j");
		output.write(reinterpret_cast<char*>(common.buffer.data()), common.buffer.size());
		BinaryReader blizzard = hierarchy.open_file("scripts/blizzard.j").value();
		std::ofstream output2("data/tools/blizzard.j");
		output2.write(reinterpret_cast<char*>(blizzard.buffer.data()), blizzard.buffer.size());

		world_edit_strings.load("UI/WorldEditStrings.txt");
		world_edit_game_strings.load("UI/WorldEditGameStrings.txt");
		world_edit_data.load("UI/WorldEditData.txt");

		world_edit_data.substitute(world_edit_game_strings, "WorldEditStrings");
		world_edit_data.substitute(world_edit_strings, "WorldEditStrings");
		trace("world edit data loaded");
	});
	trace("casc future launched");

	gl_thread_pool.init(8);
	trace("gl thread pool initialized");

	HiveWE w;
	trace("main window constructed");

	casc_future.wait();
	trace("casc future finished");

	std::println("Application start: {}ms", start_timer.elapsed_ms());

	trace("test map load begin");
	try {
		map->load("data/test map/");
		trace("test map loaded");
		trace(std::format(
			"test map stats: terrain={}x{} tilesets={} cliffs={} cliff_textures={} doodads={} special_doodads={} units={} items={}",
			map->terrain.width,
			map->terrain.height,
			map->terrain.tileset_ids.size(),
			map->terrain.cliffs.size(),
			map->terrain.cliff_textures.size(),
			map->doodads.doodads.size(),
			map->doodads.special_doodads.size(),
			map->units.units.size(),
			map->units.items.size()
		));
	} catch (const std::exception& error) {
		trace(std::format("test map load failed: {}", error.what()));
		return EXIT_FAILURE;
	} catch (...) {
		trace("test map load failed: unknown exception");
		return EXIT_FAILURE;
	}
	// map->load("C:/Users/User/Desktop/MCFC.w3x");

	return QApplication::exec();
}
