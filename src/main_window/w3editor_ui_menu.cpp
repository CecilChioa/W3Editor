#include "w3editor.h"
#include "localized_texts.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QMenuBar>
#include <QSize>
#include <QString>
#include <QToolBar>
#include <QVector>

namespace {
struct ToolbarItem {
	QString iconPath;
	QString text;
	bool separator = false;
};

QVector<ToolbarItem> buildNavigationItems() {
	return {
		{"data/icons/ribbon/open.png", QString(), false},
		{"data/icons/ribbon/save.png", QString(), false},
		{"data/icons/ribbon/saveas.png", QString(), false},
		{"", "", true},
		{"data/icons/ribbon/undo.png", QString(), false},
		{"data/icons/ribbon/redo.png", QString(), false},
		{"", "", true},
		{"data/icons/ribbon/units.png", QString(), false},
		{"data/icons/ribbon/heightmap.png", QString(), false},
		{"data/icons/ribbon/pathing.png", QString(), false},
		{"data/icons/ribbon/minimap.png", QString(), false},
	};
}
}

void W3Editor::setupMenuAndToolBar() {
	menuBar()->clear();
	if (navigationToolBar_) {
		removeToolBar(navigationToolBar_);
		navigationToolBar_->deleteLater();
		navigationToolBar_ = nullptr;
	}

	auto* fileMenu = menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuFile, chineseUi_));
	fileMenu->addAction(LocalizedTexts::text(UiTextId::ActionOpenMap, chineseUi_), this, &W3Editor::openMap);
	fileMenu->addAction(LocalizedTexts::text(UiTextId::ActionSave, chineseUi_));
	fileMenu->addAction(LocalizedTexts::text(UiTextId::ActionSaveAs, chineseUi_));
	fileMenu->addSeparator();
	fileMenu->addAction(LocalizedTexts::text(UiTextId::ActionExit, chineseUi_), this, &QWidget::close);

	menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuHome, chineseUi_));
	menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuView, chineseUi_));
	menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuMap, chineseUi_));
	menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuTools, chineseUi_));
	menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuWindow, chineseUi_));

	auto* langMenu = menuBar()->addMenu(LocalizedTexts::text(UiTextId::MenuLanguage, chineseUi_));
	auto* zh = langMenu->addAction(QStringLiteral("中文"));
	auto* en = langMenu->addAction(QStringLiteral("English"));
	connect(zh, &QAction::triggered, this, [this]() { applyLanguage(true); });
	connect(en, &QAction::triggered, this, [this]() { applyLanguage(false); });

	navigationToolBar_ = addToolBar(LocalizedTexts::text(UiTextId::ToolbarNavigation, chineseUi_));
	navigationToolBar_->setMovable(false);
	navigationToolBar_->setFloatable(false);
	navigationToolBar_->setIconSize(QSize(20, 20));
	navigationToolBar_->setToolButtonStyle(Qt::ToolButtonIconOnly);

	const auto items = buildNavigationItems();
	const QVector<UiTextId> textIds = {
		UiTextId::ActionOpenMap,
		UiTextId::ActionSave,
		UiTextId::ActionSaveAs,
		UiTextId::ActionUndo,
		UiTextId::ActionRedo,
		UiTextId::ActionUnits,
		UiTextId::ActionTerrain,
		UiTextId::ActionPathing,
		UiTextId::ActionMinimap
	};
	int textIndex = 0;
	for (const auto& item : items) {
		if (item.separator) {
			navigationToolBar_->addSeparator();
			continue;
		}
		QAction* action = navigationToolBar_->addAction(QIcon(item.iconPath), LocalizedTexts::text(textIds[textIndex++], chineseUi_));
		if (item.iconPath.endsWith("/open.png")) {
			connect(action, &QAction::triggered, this, &W3Editor::openMap);
		}
	}
}
