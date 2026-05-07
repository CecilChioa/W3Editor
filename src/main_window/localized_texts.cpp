#include "localized_texts.h"

QString LocalizedTexts::text(UiTextId id, bool chinese) {
	if (chinese) {
		switch (id) {
		case UiTextId::MenuFile: return QStringLiteral("文件");
		case UiTextId::MenuHome: return QStringLiteral("主页");
		case UiTextId::MenuView: return QStringLiteral("视图");
		case UiTextId::MenuMap: return QStringLiteral("地图");
		case UiTextId::MenuTools: return QStringLiteral("工具");
		case UiTextId::MenuWindow: return QStringLiteral("窗口");
		case UiTextId::MenuLanguage: return QStringLiteral("语言");
		case UiTextId::ActionOpenMap: return QStringLiteral("打开地图");
		case UiTextId::ActionSave: return QStringLiteral("保存");
		case UiTextId::ActionSaveAs: return QStringLiteral("另存为");
		case UiTextId::ActionExit: return QStringLiteral("退出");
		case UiTextId::ActionUndo: return QStringLiteral("撤销");
		case UiTextId::ActionRedo: return QStringLiteral("重做");
		case UiTextId::ActionUnits: return QStringLiteral("单位");
		case UiTextId::ActionTerrain: return QStringLiteral("地形");
		case UiTextId::ActionPathing: return QStringLiteral("路径");
		case UiTextId::ActionMinimap: return QStringLiteral("小地图");
		case UiTextId::ToolbarNavigation: return QStringLiteral("导航");
		case UiTextId::WindowTitle: return QStringLiteral("W3Editor");
		case UiTextId::OutputDockTitle: return QStringLiteral("输出");
		case UiTextId::DoodadPaletteTitle: return QStringLiteral("装饰物面板");
		case UiTextId::StatusReady: return QStringLiteral("就绪");
		case UiTextId::OpenMapDialogTitle: return QStringLiteral("打开地图");
		case UiTextId::OpenMapDialogFilter: return QStringLiteral("Warcraft III Map (*.w3x *.w3m *.mpq *.w3e);;All Files (*.*)");
		case UiTextId::OpenCancelled: return QStringLiteral("已取消打开。");
		case UiTextId::OutputStartupText: return QStringLiteral("W3Editor 已启动。\n点击“打开地图”并选择 .w3x/.w3m/.w3e。");
		case UiTextId::MinimapPlaceholder: return QStringLiteral("小地图");
		case UiTextId::NoPreviewData: return QStringLiteral("暂无预览数据");
		case UiTextId::DoodadIcecrownTreeWall: return QStringLiteral("冰冠树墙");
		case UiTextId::DoodadNorthrendTreeCanopy: return QStringLiteral("诺森德树冠");
		case UiTextId::DoodadNorthrendTreeWall: return QStringLiteral("诺森德树墙");
		case UiTextId::DoodadOutlandTreeWall: return QStringLiteral("外域树墙");
		case UiTextId::DoodadRuinsTreeCanopy: return QStringLiteral("废墟树冠");
		case UiTextId::DoodadVillageTreeWall: return QStringLiteral("村庄树墙");
		}
	}

	switch (id) {
	case UiTextId::MenuFile: return QStringLiteral("File");
	case UiTextId::MenuHome: return QStringLiteral("Home");
	case UiTextId::MenuView: return QStringLiteral("View");
	case UiTextId::MenuMap: return QStringLiteral("Map");
	case UiTextId::MenuTools: return QStringLiteral("Tools");
	case UiTextId::MenuWindow: return QStringLiteral("Window");
	case UiTextId::MenuLanguage: return QStringLiteral("Language");
	case UiTextId::ActionOpenMap: return QStringLiteral("Open Map");
	case UiTextId::ActionSave: return QStringLiteral("Save");
	case UiTextId::ActionSaveAs: return QStringLiteral("Save As");
	case UiTextId::ActionExit: return QStringLiteral("Exit");
	case UiTextId::ActionUndo: return QStringLiteral("Undo");
	case UiTextId::ActionRedo: return QStringLiteral("Redo");
	case UiTextId::ActionUnits: return QStringLiteral("Units");
	case UiTextId::ActionTerrain: return QStringLiteral("Terrain");
	case UiTextId::ActionPathing: return QStringLiteral("Pathing");
	case UiTextId::ActionMinimap: return QStringLiteral("Minimap");
	case UiTextId::ToolbarNavigation: return QStringLiteral("Navigation");
	case UiTextId::WindowTitle: return QStringLiteral("W3Editor");
	case UiTextId::OutputDockTitle: return QStringLiteral("Output");
	case UiTextId::DoodadPaletteTitle: return QStringLiteral("Doodad Palette");
	case UiTextId::StatusReady: return QStringLiteral("Ready");
	case UiTextId::OpenMapDialogTitle: return QStringLiteral("Open Map");
	case UiTextId::OpenMapDialogFilter: return QStringLiteral("Warcraft III Map (*.w3x *.w3m *.mpq *.w3e);;All Files (*.*)");
	case UiTextId::OpenCancelled: return QStringLiteral("Open cancelled.");
	case UiTextId::OutputStartupText: return QStringLiteral("W3Editor started.\nClick 'Open Map' and choose .w3x/.w3m/.w3e.");
	case UiTextId::MinimapPlaceholder: return QStringLiteral("Minimap");
	case UiTextId::NoPreviewData: return QStringLiteral("No preview data");
	case UiTextId::DoodadIcecrownTreeWall: return QStringLiteral("Icecrown Tree Wall");
	case UiTextId::DoodadNorthrendTreeCanopy: return QStringLiteral("Northrend Tree Canopy");
	case UiTextId::DoodadNorthrendTreeWall: return QStringLiteral("Northrend Tree Wall");
	case UiTextId::DoodadOutlandTreeWall: return QStringLiteral("Outland Tree Wall");
	case UiTextId::DoodadRuinsTreeCanopy: return QStringLiteral("Ruins Tree Canopy");
	case UiTextId::DoodadVillageTreeWall: return QStringLiteral("Village Tree Wall");
	}

	return QString{};
}
