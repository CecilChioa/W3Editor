#pragma once

#include <QMainWindow>
#include "../map_io/map_loader.h"
#include "../editor/editor_context.h"

class QPlainTextEdit;
class QPushButton;
class QLabel;
class TerrainGlWidget;
class QDockWidget;
class QListWidget;
class QToolBar;

class W3Editor : public QMainWindow {
	Q_OBJECT

public:
	explicit W3Editor(QWidget* parent = nullptr);

private:
	TerrainGlWidget* terrainView_ = nullptr;
	QLabel* previewLabel_ = nullptr;
	QPlainTextEdit* output_ = nullptr;
	QDockWidget* logDock_ = nullptr;
	QDockWidget* doodadPaletteDock_ = nullptr;
	QListWidget* doodadList_ = nullptr;
	QToolBar* navigationToolBar_ = nullptr;
	EditorContext editorContext_;
	bool chineseUi_ = true;

	void setupMenuAndToolBar();
	void openMap();
	void appendLine(const QString& text);
	void renderPreview(const MapLoadResult& result);
	void setupHiveLikeLayout();
	void setupCentralArea();
	void setupDocks();
	void rebuildMenuAndToolbarTexts();
	void applyLanguage(bool chinese);
};
