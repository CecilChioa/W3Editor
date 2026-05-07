#pragma once

#include <QString>

enum class UiTextId {
	MenuFile,
	MenuHome,
	MenuView,
	MenuMap,
	MenuTools,
	MenuWindow,
	MenuLanguage,
	ActionOpenMap,
	ActionSave,
	ActionSaveAs,
	ActionExit,
	ActionUndo,
	ActionRedo,
	ActionUnits,
	ActionTerrain,
	ActionPathing,
	ActionMinimap,
	ToolbarNavigation,
	WindowTitle,
	OutputDockTitle,
	DoodadPaletteTitle,
	StatusReady,
	OpenMapDialogTitle,
	OpenMapDialogFilter,
	OpenCancelled,
	OutputStartupText,
	MinimapPlaceholder,
	NoPreviewData,
	DoodadIcecrownTreeWall,
	DoodadNorthrendTreeCanopy,
	DoodadNorthrendTreeWall,
	DoodadOutlandTreeWall,
	DoodadRuinsTreeCanopy,
	DoodadVillageTreeWall
};

class LocalizedTexts {
public:
	static QString text(UiTextId id, bool chinese);
};
