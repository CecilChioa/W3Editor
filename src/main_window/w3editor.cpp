#include "w3editor.h"
#include "localized_texts.h"

#include <QPlainTextEdit>

W3Editor::W3Editor(QWidget* parent) : QMainWindow(parent) {
	setWindowTitle(LocalizedTexts::text(UiTextId::WindowTitle, true));
	resize(1600, 960);
	setupMenuAndToolBar();
	setupHiveLikeLayout();
	applyLanguage(true);
}

void W3Editor::appendLine(const QString& text) {
	if (output_) output_->appendPlainText(text);
}
