#include "w3editor.h"
#include "terrain_gl_widget.h"
#include "localized_texts.h"

#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

void W3Editor::setupCentralArea() {
	auto* central = new QWidget(this);
	auto* root = new QVBoxLayout(central);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	terrainView_ = new TerrainGlWidget(central);
	terrainView_->setMinimumSize(200, 200);
	root->addWidget(terrainView_, 1);

	previewLabel_ = new QLabel(terrainView_);
	previewLabel_->setWindowFlag(Qt::FramelessWindowHint, true);
	previewLabel_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
	previewLabel_->setMinimumSize(220, 220);
	previewLabel_->setMaximumSize(220, 220);
	previewLabel_->setAlignment(Qt::AlignCenter);
	previewLabel_->setText(LocalizedTexts::text(UiTextId::MinimapPlaceholder, chineseUi_));
	previewLabel_->setStyleSheet(QStringLiteral("background: rgba(30,30,30,190); border: 1px solid #4a4a4a;"));
	previewLabel_->move(14, 14);
	previewLabel_->show();

	setCentralWidget(central);
}

void W3Editor::setupDocks() {
	output_ = new QPlainTextEdit(this);
	output_->setReadOnly(true);
	output_->setPlainText(LocalizedTexts::text(UiTextId::OutputStartupText, chineseUi_));

	logDock_ = new QDockWidget(LocalizedTexts::text(UiTextId::OutputDockTitle, chineseUi_), this);
	logDock_->setAllowedAreas(Qt::BottomDockWidgetArea);
	logDock_->setWidget(output_);
	addDockWidget(Qt::BottomDockWidgetArea, logDock_);
	logDock_->setMinimumHeight(170);
	logDock_->hide();

	doodadList_ = new QListWidget(this);
	doodadList_->addItems({
		LocalizedTexts::text(UiTextId::DoodadIcecrownTreeWall, chineseUi_),
		LocalizedTexts::text(UiTextId::DoodadNorthrendTreeCanopy, chineseUi_),
		LocalizedTexts::text(UiTextId::DoodadNorthrendTreeWall, chineseUi_),
		LocalizedTexts::text(UiTextId::DoodadOutlandTreeWall, chineseUi_),
		LocalizedTexts::text(UiTextId::DoodadRuinsTreeCanopy, chineseUi_),
		LocalizedTexts::text(UiTextId::DoodadVillageTreeWall, chineseUi_)
	});

	doodadPaletteDock_ = new QDockWidget(LocalizedTexts::text(UiTextId::DoodadPaletteTitle, chineseUi_), this);
	doodadPaletteDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	doodadPaletteDock_->setWidget(doodadList_);
	addDockWidget(Qt::RightDockWidgetArea, doodadPaletteDock_);
	doodadPaletteDock_->setFloating(true);
	doodadPaletteDock_->resize(280, 520);
	doodadPaletteDock_->move(x() + width() - 340, y() + 140);
	doodadPaletteDock_->hide();
}

void W3Editor::setupHiveLikeLayout() {
	setupCentralArea();
	setupDocks();
	statusBar()->showMessage(LocalizedTexts::text(UiTextId::StatusReady, chineseUi_));
}

void W3Editor::rebuildMenuAndToolbarTexts() {
	setupMenuAndToolBar();
}

void W3Editor::applyLanguage(bool chinese) {
	chineseUi_ = chinese;
	rebuildMenuAndToolbarTexts();
	setWindowTitle(LocalizedTexts::text(UiTextId::WindowTitle, chineseUi_));
	if (previewLabel_) previewLabel_->setText(LocalizedTexts::text(UiTextId::MinimapPlaceholder, chineseUi_));
	if (output_) output_->setPlainText(LocalizedTexts::text(UiTextId::OutputStartupText, chineseUi_));
	if (logDock_) logDock_->setWindowTitle(LocalizedTexts::text(UiTextId::OutputDockTitle, chineseUi_));
	if (doodadPaletteDock_) doodadPaletteDock_->setWindowTitle(LocalizedTexts::text(UiTextId::DoodadPaletteTitle, chineseUi_));
	if (doodadList_) {
		doodadList_->clear();
		doodadList_->addItems({
			LocalizedTexts::text(UiTextId::DoodadIcecrownTreeWall, chineseUi_),
			LocalizedTexts::text(UiTextId::DoodadNorthrendTreeCanopy, chineseUi_),
			LocalizedTexts::text(UiTextId::DoodadNorthrendTreeWall, chineseUi_),
			LocalizedTexts::text(UiTextId::DoodadOutlandTreeWall, chineseUi_),
			LocalizedTexts::text(UiTextId::DoodadRuinsTreeCanopy, chineseUi_),
			LocalizedTexts::text(UiTextId::DoodadVillageTreeWall, chineseUi_)
		});
	}
	statusBar()->showMessage(LocalizedTexts::text(UiTextId::StatusReady, chineseUi_));
}
