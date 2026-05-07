#include "w3editor.h"
#include "localized_texts.h"

#include <QHash>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <algorithm>
#include <cmath>

namespace {
QPoint previewPointForWorld(const QPointF& world, const W3eHeaderInfo& terrain, const QSize& imageSize) {
	const double mapWorldWidth = std::max(1.0, static_cast<double>(terrain.width - 1) * 128.0);
	const double mapWorldHeight = std::max(1.0, static_cast<double>(terrain.height - 1) * 128.0);
	const double nx = (world.x() - terrain.offset.x()) / mapWorldWidth;
	const double ny = (world.y() - terrain.offset.y()) / mapWorldHeight;
	const int x = std::clamp(static_cast<int>(std::round(nx * (imageSize.width() - 1))), 0, imageSize.width() - 1);
	const int y = std::clamp(static_cast<int>(std::round((1.0 - ny) * (imageSize.height() - 1))), 0, imageSize.height() - 1);
	return QPoint(x, y);
}
}

void W3Editor::renderPreview(const MapLoadResult& result) {
	if (!previewLabel_) return;
	const W3eHeaderInfo& info = result.terrain;
	if (info.width <= 0 || info.height <= 0 || info.corners.isEmpty()) {
		previewLabel_->setText(LocalizedTexts::text(UiTextId::NoPreviewData, chineseUi_));
		return;
	}

	const int tileWidth = std::max(1, info.width - 1);
	const int tileHeight = std::max(1, info.height - 1);
	QImage img(tileWidth, tileHeight, QImage::Format_RGB32);
	img.fill(QColor(25, 25, 25));

	auto colorForTexture = [](const QString& key) {
		const uint h = qHash(key);
		const int r = 70 + static_cast<int>(h & 0x7F);
		const int g = 70 + static_cast<int>((h >> 7) & 0x7F);
		const int b = 70 + static_cast<int>((h >> 14) & 0x7F);
		return QColor(std::min(r, 230), std::min(g, 230), std::min(b, 230));
	};

	if (result.terrainModel.valid && !result.terrainModel.drawBatches.ground.isEmpty()) {
		for (const TerrainMaterialBatch& batch : result.terrainModel.drawBatches.ground) {
			const QColor color = colorForTexture(batch.texturePath.isEmpty() ? QStringLiteral("__missing_ground") : batch.texturePath);
			for (quint32 tileIndex : batch.tileIndices) {
				const int idx = static_cast<int>(tileIndex);
				if (idx < 0 || idx >= tileWidth * tileHeight) continue;
				const int tx = idx % tileWidth;
				const int ty = idx / tileWidth;
				img.setPixelColor(tx, ty, color);
			}
		}
	}

	for (quint32 tileIndex : result.terrainModel.drawBatches.unresolvedGroundTiles) {
		const int idx = static_cast<int>(tileIndex);
		if (idx < 0 || idx >= tileWidth * tileHeight) continue;
		const int tx = idx % tileWidth;
		const int ty = idx / tileWidth;
		img.setPixelColor(tx, ty, QColor(220, 40, 40));
	}

	for (const TerrainMaterialBatch& batch : result.terrainModel.drawBatches.cliff) {
		QColor color = colorForTexture(batch.texturePath.isEmpty() ? QStringLiteral("__missing_cliff") : batch.texturePath);
		color = color.lighter(145);
		for (quint32 tileIndex : batch.tileIndices) {
			const int idx = static_cast<int>(tileIndex);
			if (idx < 0 || idx >= tileWidth * tileHeight) continue;
			const int tx = idx % tileWidth;
			const int ty = idx / tileWidth;
			img.setPixelColor(tx, ty, color);
		}
	}

	if (result.pathing.valid && !result.pathing.flags.isEmpty()) {
		const int step = std::max(1, static_cast<int>(std::ceil(result.pathing.width / 512.0)));
		for (int py = 0; py < static_cast<int>(result.pathing.height); py += step) {
			for (int px = 0; px < static_cast<int>(result.pathing.width); px += step) {
				const int pidx = py * static_cast<int>(result.pathing.width) + px;
				if (pidx < 0 || pidx >= result.pathing.flags.size()) continue;
				const quint8 flags = result.pathing.flags[pidx];
				if ((flags & 0b00001010) == 0) continue;
				const int x = std::clamp(px * tileWidth / static_cast<int>(result.pathing.width), 0, tileWidth - 1);
				const int y = std::clamp(py * tileHeight / static_cast<int>(result.pathing.height), 0, tileHeight - 1);
				QColor base = img.pixelColor(x, y);
				img.setPixelColor(x, y, QColor(
					std::min(255, base.red() + 70),
					std::max(0, base.green() - 45),
					std::max(0, base.blue() - 45)));
			}
		}
	}

	QPainter painter(&img);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(240, 215, 70, 190));
	const int doodadStride = std::max(1, static_cast<int>(result.placedDoodads.size() / 2500));
	for (int i = 0; i < result.placedDoodads.size(); i += doodadStride) {
		const QPoint p = previewPointForWorld(result.placedDoodads[i].position, info, img.size());
		painter.drawRect(p.x(), p.y(), 1, 1);
	}

	painter.setBrush(QColor(235, 75, 75, 220));
	for (const PlacedUnit& unit : result.placedUnits) {
		const QPoint p = previewPointForWorld(unit.position, info, img.size());
		painter.drawRect(p.x() - 1, p.y() - 1, 3, 3);
	}
	painter.end();

	const QPixmap pix = QPixmap::fromImage(img).scaled(
		previewLabel_->size(),
		Qt::KeepAspectRatio,
		Qt::FastTransformation);
	previewLabel_->setPixmap(pix);
}
