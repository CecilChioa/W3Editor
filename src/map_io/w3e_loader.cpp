#include "w3e_loader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

// The vcpkg StormLib port used here is built with ANSI paths unless the port is
// overlaid with STORM_UNICODE=ON. Keep this translation unit's declarations in
// ANSI mode even though the Qt app itself is built with UNICODE.
#if defined(UNICODE)
#define W3EDITOR_RESTORE_UNICODE
#undef UNICODE
#endif
#if defined(_UNICODE)
#define W3EDITOR_RESTORE__UNICODE
#undef _UNICODE
#endif
#include <StormLib.h>
#if defined(W3EDITOR_RESTORE_UNICODE)
#define UNICODE
#undef W3EDITOR_RESTORE_UNICODE
#endif
#if defined(W3EDITOR_RESTORE__UNICODE)
#define _UNICODE
#undef W3EDITOR_RESTORE__UNICODE
#endif

#include <limits>
#include <cstring>

namespace {
quint16 readLE16(const unsigned char* p) {
	return static_cast<quint16>(p[0]) |
		(static_cast<quint16>(p[1]) << 8);
}

qint32 readLE32(const unsigned char* p) {
	return static_cast<qint32>(p[0]) |
		(static_cast<qint32>(p[1]) << 8) |
		(static_cast<qint32>(p[2]) << 16) |
		(static_cast<qint32>(p[3]) << 24);
}

float readLEFloat(const unsigned char* p) {
	quint32 bits = static_cast<quint32>(p[0]) |
		(static_cast<quint32>(p[1]) << 8) |
		(static_cast<quint32>(p[2]) << 16) |
		(static_cast<quint32>(p[3]) << 24);
	float value = 0.0f;
	static_assert(sizeof(value) == sizeof(bits));
	memcpy(&value, &bits, sizeof(value));
	return value;
}

QString readFourCC(const unsigned char* p) {
	return QString::fromLatin1(reinterpret_cast<const char*>(p), 4);
}

bool advanceChecked(qsizetype& off, qsizetype delta, qsizetype size) {
	if (delta < 0) return false;
	if (off > size || delta > (size - off)) return false;
	off += delta;
	return true;
}

W3eHeaderInfo loadFromDirectory(const QString& directoryPath) {
	W3eHeaderInfo info;
	info.sourcePath = directoryPath;

	const QString w3ePath = QDir(directoryPath).filePath("war3map.w3e");
	QFile f(w3ePath);
	if (!f.open(QIODevice::ReadOnly)) {
		info.error = QStringLiteral("Cannot read war3map.w3e in folder: %1").arg(w3ePath);
		return info;
	}
	return W3eLoader::parseW3eFromBytes(f.readAll(), w3ePath);
}

W3eHeaderInfo loadFromArchive(const QString& archivePath) {
	W3eHeaderInfo info;
	info.sourcePath = archivePath;

	HANDLE mpq = nullptr;
	const QByteArray archivePathBytes = QDir::toNativeSeparators(archivePath).toLocal8Bit();
	if (!SFileOpenArchive(archivePathBytes.constData(), 0, MPQ_OPEN_READ_ONLY, &mpq)) {
		info.error = QStringLiteral("Failed to open archive: %1 (GetLastError=%2)")
			.arg(archivePath)
			.arg(static_cast<qulonglong>(GetLastError()));
		return info;
	}

	HANDLE file = nullptr;
	if (!SFileOpenFileEx(mpq, "war3map.w3e", SFILE_OPEN_FROM_MPQ, &file)) {
		SFileCloseArchive(mpq);
		info.error = QStringLiteral("war3map.w3e not found in archive.");
		return info;
	}

	DWORD size = SFileGetFileSize(file, nullptr);
	if (size == SFILE_INVALID_SIZE || size == 0) {
		SFileCloseFile(file);
		SFileCloseArchive(mpq);
		info.error = QStringLiteral("Invalid war3map.w3e size in archive.");
		return info;
	}

	QByteArray bytes;
	bytes.resize(static_cast<int>(size));
	DWORD read = 0;
	if (!SFileReadFile(file, bytes.data(), size, &read, nullptr) || read != size) {
		SFileCloseFile(file);
		SFileCloseArchive(mpq);
		info.error = QStringLiteral("Failed to read war3map.w3e from archive.");
		return info;
	}

	SFileCloseFile(file);
	SFileCloseArchive(mpq);
	return W3eLoader::parseW3eFromBytes(bytes, archivePath + "/war3map.w3e");
}
}

W3eHeaderInfo W3eLoader::load(const QString& path) {
	const QFileInfo fi(path);
	if (fi.isDir()) {
		return loadFromDirectory(path);
	}

	const QString suffix = fi.suffix().toLower();
	if (suffix == "w3x" || suffix == "w3m" || suffix == "mpq") {
		return loadFromArchive(path);
	}

	if (fi.fileName().compare("war3map.w3e", Qt::CaseInsensitive) == 0) {
		QFile f(path);
		W3eHeaderInfo info;
		info.sourcePath = path;
		if (!f.open(QIODevice::ReadOnly)) {
			info.error = QStringLiteral("Cannot read file: %1").arg(path);
			return info;
		}
		return parseW3eFromBytes(f.readAll(), path);
	}

	W3eHeaderInfo info;
	info.sourcePath = path;
	info.error = QStringLiteral("Unsupported input. Use .w3x/.w3m/.mpq or war3map.w3e.");
	return info;
}

W3eHeaderInfo W3eLoader::parseW3eFromBytes(const QByteArray& bytes, const QString& sourcePath) {
	W3eHeaderInfo info;
	info.sourcePath = sourcePath;

	if (bytes.size() < 32) {
		info.error = QStringLiteral("war3map.w3e is too short.");
		return info;
	}

	const unsigned char* p = reinterpret_cast<const unsigned char*>(bytes.constData());
	if (!(p[0] == 'W' && p[1] == '3' && p[2] == 'E' && p[3] == '!')) {
		info.error = QStringLiteral("Invalid W3E signature (expected W3E!).");
		return info;
	}

	qsizetype off = 4;
	info.version = readLE32(p + off);
	if (!advanceChecked(off, 4, bytes.size())) { info.error = QStringLiteral("Truncated after version."); return info; }

	info.tileset = QString(QChar(static_cast<char>(p[off])));
	if (!advanceChecked(off, 1, bytes.size())) { info.error = QStringLiteral("Truncated after tileset."); return info; }

	info.customTileset = readLE32(p + off);
	if (!advanceChecked(off, 4, bytes.size())) { info.error = QStringLiteral("Truncated after customTileset."); return info; }

	info.groundTileCount = readLE32(p + off);
	if (!advanceChecked(off, 4, bytes.size())) { info.error = QStringLiteral("Truncated after groundTileCount."); return info; }
	if (info.groundTileCount < 0 || info.groundTileCount > 1024) {
		info.error = QStringLiteral("Invalid groundTileCount: %1").arg(info.groundTileCount);
		return info;
	}
	info.groundTileIds.reserve(info.groundTileCount);
	for (qint32 i = 0; i < info.groundTileCount; ++i) {
		if (!advanceChecked(off, 4, bytes.size())) {
			info.error = QStringLiteral("Truncated in ground tiles.");
			return info;
		}
		info.groundTileIds.push_back(readFourCC(p + off - 4));
	}

	info.cliffTileCount = readLE32(p + off);
	if (!advanceChecked(off, 4, bytes.size())) { info.error = QStringLiteral("Truncated after cliffTileCount."); return info; }
	if (info.cliffTileCount < 0 || info.cliffTileCount > 1024) {
		info.error = QStringLiteral("Invalid cliffTileCount: %1").arg(info.cliffTileCount);
		return info;
	}
	info.cliffTileIds.reserve(info.cliffTileCount);
	for (qint32 i = 0; i < info.cliffTileCount; ++i) {
		if (!advanceChecked(off, 4, bytes.size())) {
			info.error = QStringLiteral("Truncated in cliff tiles.");
			return info;
		}
		info.cliffTileIds.push_back(readFourCC(p + off - 4));
	}

	info.width = readLE32(p + off);
	if (!advanceChecked(off, 4, bytes.size())) { info.error = QStringLiteral("Truncated after width."); return info; }
	info.height = readLE32(p + off);
	if (!advanceChecked(off, 4, bytes.size())) { info.error = QStringLiteral("Truncated after height."); return info; }

	info.offset = QPointF(readLEFloat(p + off), readLEFloat(p + off + 4));
	if (!advanceChecked(off, 8, bytes.size())) { info.error = QStringLiteral("Truncated after center offset."); return info; }

	if (info.width <= 0 || info.height <= 0 || info.width > 16384 || info.height > 16384) {
		info.error = QStringLiteral("Invalid map size: %1 x %2").arg(info.width).arg(info.height);
		return info;
	}

	const qsizetype cornerCount = static_cast<qsizetype>(info.width) * static_cast<qsizetype>(info.height);
	const qsizetype bytesPerCorner = info.version >= 12 ? 8 : 7;
	const qsizetype cornerBytes = cornerCount * bytesPerCorner;
	if (!advanceChecked(off, cornerBytes, bytes.size())) {
		info.error = QStringLiteral("Truncated in corner data: need %1 bytes.").arg(cornerBytes);
		return info;
	}

	info.corners.resize(static_cast<int>(cornerCount));
	info.minCornerHeight = std::numeric_limits<float>::max();
	info.maxCornerHeight = std::numeric_limits<float>::lowest();
	info.minWaterHeight = std::numeric_limits<float>::max();
	info.maxWaterHeight = std::numeric_limits<float>::lowest();

	const qsizetype start = off - cornerBytes;
	for (qsizetype i = 0; i < cornerCount; ++i) {
		const unsigned char* c = p + start + i * bytesPerCorner;
		W3eCorner corner;
		corner.rawHeight = readLE16(c);
		corner.height = (static_cast<float>(corner.rawHeight) - 8192.0f) / 512.0f;

		corner.rawWaterAndEdge = readLE16(c + 2);
		corner.waterHeight = (static_cast<float>(corner.rawWaterAndEdge & 0x3FFF) - 8192.0f) / 512.0f;
		corner.mapEdge = (corner.rawWaterAndEdge & 0x4000) != 0;

		if (info.version >= 12) {
			const quint16 textureAndFlags = readLE16(c + 4);
			corner.groundTexture = textureAndFlags & 0b00'0000'0011'1111;
			corner.ramp = (textureAndFlags & 0b00'0100'0000) != 0;
			corner.blight = (textureAndFlags & 0b00'1000'0000) != 0;
			corner.water = (textureAndFlags & 0b01'0000'0000) != 0;
			corner.boundary = (textureAndFlags & 0b10'0000'0000) != 0;
		} else {
			const quint8 textureAndFlags = c[4];
			corner.groundTexture = textureAndFlags & 0b0000'1111;
			corner.ramp = (textureAndFlags & 0b0001'0000) != 0;
			corner.blight = (textureAndFlags & 0b0010'0000) != 0;
			corner.water = (textureAndFlags & 0b0100'0000) != 0;
			corner.boundary = (textureAndFlags & 0b1000'0000) != 0;
		}

		const quint8 variation = c[info.version >= 12 ? 6 : 5];
		corner.groundVariation = variation & 0b0001'1111;
		corner.cliffVariation = (variation & 0b1110'0000) >> 5;

		const quint8 misc = c[info.version >= 12 ? 7 : 6];
		corner.cliffTexture = (misc & 0b1111'0000) >> 4;
		corner.layerHeight = misc & 0b0000'1111;

		info.corners[static_cast<int>(i)] = corner;
		info.minCornerHeight = std::min(info.minCornerHeight, corner.height);
		info.maxCornerHeight = std::max(info.maxCornerHeight, corner.height);
		info.minWaterHeight = std::min(info.minWaterHeight, corner.waterHeight);
		info.maxWaterHeight = std::max(info.maxWaterHeight, corner.waterHeight);
		if (corner.water) ++info.waterCornerCount;
		if (corner.boundary) ++info.boundaryCornerCount;
		if (corner.ramp) ++info.rampCornerCount;
		if (corner.blight) ++info.blightCornerCount;
		if (corner.mapEdge) ++info.mapEdgeCornerCount;
	}

	info.valid = true;
	return info;
}
