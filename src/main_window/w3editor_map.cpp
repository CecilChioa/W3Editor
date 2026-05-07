#include "w3editor.h"
#include "map_load_report_formatter.h"
#include "terrain_gl_widget.h"
#include "localized_texts.h"

#include <QFileDialog>
#include <QProgressDialog>
#include <QApplication>

void W3Editor::openMap() {
	const QString path = QFileDialog::getOpenFileName(
		this,
		LocalizedTexts::text(UiTextId::OpenMapDialogTitle, chineseUi_),
		QString(),
		LocalizedTexts::text(UiTextId::OpenMapDialogFilter, chineseUi_));

	if (path.isEmpty()) {
		appendLine(LocalizedTexts::text(UiTextId::OpenCancelled, chineseUi_));
		return;
	}

	QProgressDialog progress(QStringLiteral("正在加载地图..."), QString(), 0, 100, this);
	progress.setWindowModality(Qt::WindowModal);
	progress.setCancelButton(nullptr);
	progress.setMinimumDuration(0);
	progress.setValue(5);
	qApp->processEvents();

	progress.setLabelText(QStringLiteral("读取地图与解析数据..."));
	progress.setValue(35);
	qApp->processEvents();

	const auto result = MapLoader::load(path);
	editorContext_.setLoadedMap(result);

	progress.setLabelText(QStringLiteral("构建地形与预览..."));
	progress.setValue(75);
	qApp->processEvents();

	const MapLoadReport report = MapLoadReportFormatter::format(path, result);
	for (const QString& line : report.lines) appendLine(line);
	if (!report.success) {
		appendLine(report.failureLine);
		progress.setValue(100);
		return;
	}

	if (terrainView_) terrainView_->setTerrainModel(result.terrainModel);
	renderPreview(result);
	progress.setValue(100);
}
