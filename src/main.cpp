#include <QApplication>
#include <QFile>
#include "main_window/w3editor.h"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	QFile themeFile(QStringLiteral("data/themes/Dark.qss"));
	if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		app.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
	}
	W3Editor w;
	w.show();
	return app.exec();
}
