#include "map_loader.h"

#include "binary_reader.h"
#include "game_data_loader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QSet>
#include <QHash>
#include <algorithm>
#include <cstring>

// Match the existing StormLib usage in w3e_loader.cpp: this vcpkg build exposes
// ANSI TCHAR entry points even though the Qt application is built as UNICODE.
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

namespace {
class MapFileSource {
public:
	~MapFileSource() {
		if (archive_) {
			SFileCloseArchive(archive_);
		}
	}

	bool open(const QString& path, QString* error) {
		inputPath_ = path;
		const QFileInfo fi(path);
		if (fi.isDir()) {
			directory_ = fi.absoluteFilePath();
			return true;
		}

		const QString suffix = fi.suffix().toLower();
		if (suffix == "w3x" || suffix == "w3m" || suffix == "mpq") {
			const QByteArray archivePath = QDir::toNativeSeparators(fi.absoluteFilePath()).toLocal8Bit();
			if (!SFileOpenArchive(archivePath.constData(), 0, MPQ_OPEN_READ_ONLY, &archive_)) {
				*error = QStringLiteral("Failed to open archive: %1 (GetLastError=%2)")
					.arg(path)
					.arg(static_cast<qulonglong>(GetLastError()));
				return false;
			}
			archivePath_ = fi.absoluteFilePath();
			archiveBacked_ = true;
			if (!unpackArchive(error)) {
				SFileCloseArchive(archive_);
				archive_ = nullptr;
				return false;
			}
			SFileCloseArchive(archive_);
			archive_ = nullptr;
			return true;
		}

		if (fi.fileName().compare("war3map.w3e", Qt::CaseInsensitive) == 0) {
			directory_ = fi.absolutePath();
			return true;
		}

		*error = QStringLiteral("Unsupported input. Use .w3x/.w3m/.mpq/.w3e or an unpacked map folder.");
		return false;
	}

	bool isArchive() const { return archiveBacked_; }
	QString mapDirectory() const { return directory_; }
	QString archivePath() const { return archivePath_; }
	QString sourcePathFor(const QString& name) const { return mapDirectory() + QStringLiteral("/") + name; }

	bool readFile(const QString& name, QByteArray* bytes, QString* error = nullptr) const {
		bytes->clear();
		QFile file(QDir(directory_).filePath(name));
		if (!file.open(QIODevice::ReadOnly)) {
			if (error) *error = QStringLiteral("%1 not found in map directory.").arg(name);
			return false;
		}
		*bytes = file.readAll();
		return true;
	}

private:
	QString unpackDirectoryForArchive() const {
		const QFileInfo fi(archivePath_);
		const QByteArray key = QCryptographicHash::hash(fi.absoluteFilePath().toUtf8(), QCryptographicHash::Sha1).toHex().left(10);
		const QString folder = fi.completeBaseName() + QStringLiteral("_") + QString::fromLatin1(key);
		return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("extracted_maps/%1").arg(folder));
	}

	bool extractOne(const QString& archiveName, const QString& outputRoot) const {
		const QString relativePath = QDir::fromNativeSeparators(archiveName);
		if (relativePath.isEmpty() || relativePath.contains(QStringLiteral(".."))) {
			return false;
		}

		const QString targetPath = QDir(outputRoot).filePath(relativePath);
		QDir().mkpath(QFileInfo(targetPath).absolutePath());

		const QByteArray source = archiveName.toLocal8Bit();
		const QByteArray target = QDir::toNativeSeparators(targetPath).toLocal8Bit();
		return SFileExtractFile(archive_, source.constData(), target.constData(), 0);
	}

	bool unpackArchive(QString* error) {
		directory_ = unpackDirectoryForArchive();
		if (!QDir().mkpath(directory_)) {
			*error = QStringLiteral("Failed to create unpack directory: %1").arg(directory_);
			return false;
		}

		int extracted = 0;
		SFILE_FIND_DATA findData = {};
		HANDLE find = SFileFindFirstFile(archive_, "*", &findData, nullptr);
		if (find) {
			do {
				const QString archiveName = QString::fromLocal8Bit(findData.cFileName);
				if (extractOne(archiveName, directory_)) {
					++extracted;
				}
			} while (SFileFindNextFile(find, &findData));
			SFileFindClose(find);
		}

		// Some maps do not expose a complete file list. Extract the files used by
		// HiveWE's load path by known names so the folder loader still works.
		const QStringList knownFiles = {
			QStringLiteral("war3map.w3i"),
			QStringLiteral("war3map.w3e"),
			QStringLiteral("war3map.wpm"),
			QStringLiteral("war3map.doo"),
			QStringLiteral("war3mapUnits.doo"),
			QStringLiteral("war3map.w3r"),
			QStringLiteral("war3map.w3c"),
			QStringLiteral("war3map.w3s"),
			QStringLiteral("war3map.imp"),
			QStringLiteral("war3map.mmp"),
			QStringLiteral("war3map.shd"),
			QStringLiteral("war3mapMisc.txt"),
			QStringLiteral("war3map.w3d"), QStringLiteral("war3mapSkin.w3d"),
			QStringLiteral("war3map.w3b"), QStringLiteral("war3mapSkin.w3b"),
			QStringLiteral("war3map.w3u"), QStringLiteral("war3mapSkin.w3u"),
			QStringLiteral("war3map.w3t"), QStringLiteral("war3mapSkin.w3t"),
			QStringLiteral("war3map.w3a"), QStringLiteral("war3mapSkin.w3a"),
			QStringLiteral("war3map.w3h"), QStringLiteral("war3mapSkin.w3h"),
			QStringLiteral("war3map.w3q"), QStringLiteral("war3mapSkin.w3q"),
			QStringLiteral("war3map.j"),
			QStringLiteral("war3map.wtg"),
			QStringLiteral("war3map.wct"),
			QStringLiteral("war3mapImported\\*")
		};
		for (const QString& fileName : knownFiles) {
			if (extractOne(fileName, directory_)) {
				++extracted;
			}
		}

		if (extracted == 0) {
			*error = QStringLiteral("Archive opened but no map files could be unpacked: %1").arg(archivePath_);
			return false;
		}
		return true;
	}

	QString inputPath_;
	QString directory_;
	QString archivePath_;
	HANDLE archive_ = nullptr;
	bool archiveBacked_ = false;
};

MapArchiveInfo parseMapArchiveInfo(const QString& path) {
	MapArchiveInfo info;
	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		info.error = QStringLiteral("failed to open map archive for header scan");
		return info;
	}

	const QByteArray data = file.readAll();
	info.present = true;
	BinaryReader r(data);
	const QString header = r.fourcc();
	if (header == QStringLiteral("HM3W")) {
		info.validHeader = true;
		info.placeholder = r.i32();
		info.name = r.cstring();
		info.flags = r.u32();
		info.playersCount = r.i32();
	} else {
		info.error = QStringLiteral("HM3W header not found");
	}

	const int dataSize = static_cast<int>(data.size());
	const int searchStart = std::max(0, dataSize - 4096);
	int footerOffset = -1;
	for (int i = dataSize - 260; i >= searchStart; --i) {
		if (i >= 0 && std::memcmp(data.constData() + i, "NGIS", 4) == 0) {
			footerOffset = i;
			break;
		}
	}
	if (footerOffset >= 0 && footerOffset + 260 <= dataSize) {
		info.validFooter = true;
		info.authentication.reserve(256);
		for (int i = 0; i < 256; ++i) {
			info.authentication.push_back(static_cast<quint8>(data.at(footerOffset + 4 + i)));
		}
	}
	return info;
}

QString status(bool ok) {
	return ok ? QStringLiteral("ok") : QStringLiteral("missing");
}

bool skipItemSets(BinaryReader& r) {
	const quint32 setCount = r.u32();
	for (quint32 i = 0; i < setCount && r.ok(); ++i) {
		const quint32 itemCount = r.u32();
		for (quint32 j = 0; j < itemCount && r.ok(); ++j) {
			r.skip(4);
			r.skip(4);
		}
	}
	return r.ok();
}

QString fourccOrEmpty(BinaryReader& r) {
	const QString id = r.fourcc();
	return id == QStringLiteral("\0\0\0\0") ? QString() : id;
}

QColor readRgba(BinaryReader& r) {
	const int rValue = r.u8();
	const int gValue = r.u8();
	const int bValue = r.u8();
	const int aValue = r.u8();
	return QColor(rValue, gValue, bValue, aValue);
}

QColor readBgraPadded(BinaryReader& r) {
	const int bValue = r.u8();
	const int gValue = r.u8();
	const int rValue = r.u8();
	r.u8();
	return QColor(rValue, gValue, bValue);
}

RandomTableSet readItemSet(BinaryReader& r) {
	RandomTableSet set;
	const quint32 itemCount = r.u32();
	for (quint32 i = 0; i < itemCount && r.ok(); ++i) {
		RandomTableItem item;
		item.id = r.fourcc();
		item.chancePercent = r.i32();
		set.items.push_back(item);
	}
	return set;
}

qint32 readSigned24(BinaryReader& r) {
	const quint8 b0 = r.u8();
	const quint8 b1 = r.u8();
	const quint8 b2 = r.u8();
	qint32 value = static_cast<qint32>(b0) |
		(static_cast<qint32>(b1) << 8) |
		(static_cast<qint32>(b2) << 16);
	if (value & 0x00800000) {
		value |= 0xFF000000;
	}
	return value;
}

W3iInfo parseW3i(const QByteArray& bytes, MapMetadata* metadata) {
	W3iInfo info;
	BinaryReader r(bytes);
	info.version = r.u32();
	if (info.version >= 18) {
		info.mapVersion = r.u32();
		info.editorVersion = r.u32();
		if (info.version >= 28) {
			info.gameVersionMajor = r.u32();
			info.gameVersionMinor = r.u32();
			info.gameVersionPatch = r.u32();
			info.gameVersionBuild = r.u32();
		}
	}
	info.name = r.cstring();
	info.author = r.cstring();
	info.description = r.cstring();
	info.suggestedPlayers = r.cstring();
	for (int i = 0; i < 8; ++i) {
		metadata->cameraBounds.push_back(r.f32());
	}
	for (int i = 0; i < 4; ++i) {
		metadata->cameraBoundsPadding.push_back(r.i32());
	}
	info.playableWidth = r.u32();
	info.playableHeight = r.u32();
	info.flags = r.u32();
	metadata->customLightEnvironmentTilesetId = QString(QChar(static_cast<char>(r.u8())));

	if (info.version >= 25) {
		metadata->loadingScreenPresetIndex = r.i32();
		metadata->loadingScreenCustomPath = r.cstring();
		metadata->loadingScreenText = r.cstring();
		metadata->loadingScreenTitle = r.cstring();
		metadata->loadingScreenSubtitle = r.cstring();
		metadata->gameDataSetIndex = r.i32();
		metadata->prologueScreenPath = r.cstring();
		metadata->prologueScreenText = r.cstring();
		metadata->prologueScreenTitle = r.cstring();
		metadata->prologueScreenSubtitle = r.cstring();
		metadata->terrainFogStyle = r.i32();
		metadata->terrainFogStartZ = r.f32();
		metadata->terrainFogEndZ = r.f32();
		metadata->terrainFogDensity = r.f32();
		metadata->terrainFogColor = readRgba(r);
		metadata->globalWeatherId = fourccOrEmpty(r);
		metadata->customSoundEnvironment = r.cstring();
		metadata->customLightEnvironmentTilesetId = QString(QChar(static_cast<char>(r.u8())));
		metadata->waterColor = readRgba(r);
		if (info.version >= 28) {
			r.skip(4);
		}
		if (info.version >= 31) {
			r.skip(8);
		}
		if (info.version >= 32) {
			r.skip(8);
			if (info.version >= 33) {
				r.skip(4);
			}
		}
	} else if (info.version == 18) {
		metadata->loadingScreenPresetIndex = r.i32();
		metadata->loadingScreenCustomPath = r.cstring();
		metadata->loadingScreenText = r.cstring();
		metadata->loadingScreenTitle = r.cstring();
		metadata->gameDataSetIndex = r.i32();
		metadata->prologueScreenPath = r.cstring();
		metadata->prologueScreenText = r.cstring();
		metadata->prologueScreenTitle = r.cstring();
	} else {
		r.skip(1);
		metadata->loadingScreenText = r.cstring();
		metadata->loadingScreenTitle = r.cstring();
		metadata->loadingScreenSubtitle = r.cstring();
		metadata->gameDataSetIndex = r.i32();
	}

	if (r.remaining() >= 4) {
		info.players = r.u32();
		for (quint32 i = 0; i < info.players && r.ok(); ++i) {
			MapPlayerInfo player;
			player.id = r.i32();
			player.type = r.i32();
			player.race = r.i32();
			player.fixedStartPosition = r.i32();
			player.name = r.cstring();
			player.startPosition = QPointF(r.f32(), r.f32());
			player.allyLowPriorityFlags = r.u32();
			player.allyHighPriorityFlags = r.u32();
			if (info.version >= 31) {
				r.skip(8);
			}
			metadata->players.push_back(player);
		}
	}
	if (r.remaining() >= 4) {
		info.forces = r.u32();
		for (quint32 i = 0; i < info.forces && r.ok(); ++i) {
			MapForceInfo force;
			force.flags = r.u32();
			force.playerMaskFlags = r.u32();
			force.name = r.cstring();
			metadata->forces.push_back(force);
		}
	}
	if (r.remaining() >= 4) {
		info.upgrades = r.u32();
		for (quint32 i = 0; i < info.upgrades && r.ok(); ++i) {
			MapUpgradeAvailabilityChange change;
			change.playerFlags = r.u32();
			change.upgradeId = r.fourcc();
			change.levelChanged = r.i32();
			change.availability = r.i32();
			metadata->upgradeAvailabilityChanges.push_back(change);
		}
	}
	if (r.remaining() >= 4) {
		info.tech = r.u32();
		for (quint32 i = 0; i < info.tech && r.ok(); ++i) {
			MapTechAvailabilityChange change;
			change.playerFlags = r.u32();
			change.techId = r.fourcc();
			metadata->techAvailabilityChanges.push_back(change);
		}
	}
	if (r.remaining() >= 4) {
		info.randomUnitTables = r.u32();
		for (quint32 i = 0; i < info.randomUnitTables && r.ok(); ++i) {
			RandomUnitTableInfo table;
			table.index = r.i32();
			table.name = r.cstring();
			const quint32 positions = r.u32();
			for (quint32 position = 0; position < positions && r.ok(); ++position) {
				table.positions.push_back(r.i32());
			}
			const quint32 lines = r.u32();
			for (quint32 line = 0; line < lines && r.ok(); ++line) {
				RandomTableItem item;
				item.chancePercent = r.i32();
				for (quint32 position = 0; position < positions && r.ok(); ++position) {
					item.ids.push_back(r.fourcc());
				}
				table.units.push_back(item);
			}
			metadata->randomUnitTables.push_back(table);
		}
	}
	if (info.version >= 25 && r.remaining() >= 4) {
		info.randomItemTables = r.u32();
		for (quint32 i = 0; i < info.randomItemTables && r.ok(); ++i) {
			RandomItemTableInfo table;
			table.index = r.i32();
			table.name = r.cstring();
			const quint32 setCount = r.u32();
			for (quint32 set = 0; set < setCount && r.ok(); ++set) {
				table.sets.push_back(readItemSet(r));
			}
			metadata->randomItemTables.push_back(table);
		}
	}
	if (!r.ok()) {
		info.error = QStringLiteral("war3map.w3i is truncated.");
		return info;
	}
	info.valid = true;
	return info;
}

WpmInfo parseWpm(const QByteArray& bytes) {
	WpmInfo info;
	BinaryReader r(bytes);
	const QString magic = r.fourcc();
	if (magic != QStringLiteral("MP3W")) {
		info.error = QStringLiteral("Invalid WPM signature: %1").arg(magic);
		return info;
	}
	info.version = r.u32();
	info.width = r.u32();
	info.height = r.u32();
	const quint64 count = static_cast<quint64>(info.width) * static_cast<quint64>(info.height);
	if (!r.ok() || count > static_cast<quint64>(r.remaining())) {
		info.error = QStringLiteral("war3map.wpm is truncated.");
		return info;
	}
	info.cells = static_cast<quint32>(count);
	info.flags.reserve(static_cast<qsizetype>(count));
	for (quint64 i = 0; i < count; ++i) {
		const quint8 flags = r.u8();
		info.flags.push_back(flags);
		if (flags & 0b00000010) ++info.canWalk;
		if (flags & 0b00000100) ++info.canFly;
		if (flags & 0b00001000) ++info.canBuild;
		if (flags & 0b01000000) ++info.ground;
		if (!(flags & 0b00000010)) ++info.unwalkable;
		if (!(flags & 0b00000100)) ++info.unflyable;
		if (!(flags & 0b00001000)) ++info.unbuildable;
		if (!(flags & 0b01000000)) ++info.water;
	}
	info.valid = r.ok();
	if (!info.valid) {
		info.error = QStringLiteral("war3map.wpm is truncated.");
	}
	return info;
}

FileLoadStatus parseDoodads(
	const QByteArray& bytes,
	int gameVersionMajor,
	int gameVersionMinor,
	QVector<PlacedDoodad>* placedDoodads,
	QVector<TerrainDoodad>* terrainDoodads
) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const QString magic = r.fourcc();
	const quint32 version = r.u32();
	const quint32 subversion = r.u32();
	const quint32 count = r.u32();
	if (!r.ok() || magic != QStringLiteral("W3do")) {
		status.error = QStringLiteral("invalid signature");
		return status;
	}
	quint32 itemSetCount = 0;
	const bool hasSkinId = gameVersionMajor * 100 + gameVersionMinor >= 132;
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		PlacedDoodad placed;
		placed.id = r.fourcc();
		placed.variation = r.u32();
		const float x = r.f32();
		const float y = r.f32();
		placed.z = r.f32();
		placed.position = QPointF(x, y);
		placed.angle = r.f32();
		placed.scale = QPointF(r.f32(), r.f32());
		placed.scaleZ = r.f32();
		if (hasSkinId) {
			r.skip(4);
		}
		placed.visibility = r.u8();
		placed.lifePercent = r.u8();
		if (version >= 8) {
			placed.droppedItemTableIndex = r.i32();
			const quint32 sets = r.u32();
			itemSetCount += sets;
			for (quint32 set = 0; set < sets && r.ok(); ++set) {
				placed.droppedItemSets.push_back(readItemSet(r));
			}
		}
		placed.creationNumber = r.u32();
		if (placedDoodads) {
			placedDoodads->push_back(placed);
		}
	}
	const quint32 specialVersion = r.u32();
	const quint32 specialCount = r.u32();
	for (quint32 i = 0; i < specialCount && r.ok(); ++i) {
		TerrainDoodad doodad;
		doodad.id = r.fourcc();
		doodad.z = r.i32();
		doodad.x = r.i32();
		doodad.y = r.i32();
		if (terrainDoodads) {
			terrainDoodads->push_back(doodad);
		}
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated doodads");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 subversion=%2 doodads=%3 itemSets=%4 specialVersion=%5 special=%6")
		.arg(version)
		.arg(subversion)
		.arg(count)
		.arg(itemSetCount)
		.arg(specialVersion)
		.arg(specialCount);
	return status;
}

FileLoadStatus parseUnits(
	const QByteArray& bytes,
	int gameVersionMajor,
	int gameVersionMinor,
	QVector<PlacedUnit>* placedUnits
) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const QString magic = r.fourcc();
	const quint32 version = r.u32();
	const quint32 subversion = r.u32();
	const quint32 count = r.u32();
	if (!r.ok() || magic != QStringLiteral("W3do")) {
		status.error = QStringLiteral("invalid signature");
		return status;
	}
	const bool hasSkinId = gameVersionMajor * 100 + gameVersionMinor >= 132;
	quint32 itemSetCount = 0;
	quint32 inventoryItems = 0;
	quint32 modifiedAbilities = 0;
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		PlacedUnit placed;
		placed.id = r.fourcc();
		placed.variation = r.u32();
		const float x = r.f32();
		const float y = r.f32();
		placed.z = r.f32();
		placed.position = QPointF(x, y);
		placed.angle = r.f32();
		placed.scale = QPointF(r.f32(), r.f32());
		placed.scaleZ = r.f32();
		if (hasSkinId) {
			r.skip(4);
		}
		placed.visibility = r.u8();
		placed.player = r.u32();
		placed.unknown1 = r.u8();
		placed.unknown2 = r.u8();
		placed.hitpoints = r.i32();
		placed.mana = r.i32();
		if (subversion >= 11) {
			placed.droppedItemTableIndex = r.i32();
		}
		const quint32 sets = r.u32();
		itemSetCount += sets;
		for (quint32 set = 0; set < sets && r.ok(); ++set) {
			placed.droppedItemSets.push_back(readItemSet(r));
		}
		placed.gold = r.i32();
		placed.targetAcquisitionRange = r.f32();
		placed.heroLevel = r.i32();
		if (subversion >= 11) {
			placed.heroStrength = r.i32();
			placed.heroAgility = r.i32();
			placed.heroIntelligence = r.i32();
		}
		const quint32 inventory = r.u32();
		inventoryItems += inventory;
		for (quint32 item = 0; item < inventory && r.ok(); ++item) {
			RandomTableItem inventoryItem;
			inventoryItem.chancePercent = r.i32();
			inventoryItem.id = r.fourcc();
			placed.inventoryItems.push_back(inventoryItem);
		}
		const quint32 abilities = r.u32();
		modifiedAbilities += abilities;
		for (quint32 ability = 0; ability < abilities && r.ok(); ++ability) {
			RandomTableItem abilityItem;
			abilityItem.id = r.fourcc();
			abilityItem.chancePercent = r.i32();
			abilityItem.ids.push_back(QString::number(r.i32()));
			placed.abilityModifications.push_back(abilityItem);
		}
		placed.randomType = r.i32();
		switch (placed.randomType) {
			case 0:
				placed.randomAnyLevel = readSigned24(r);
				placed.randomAnyItemClass = r.u8();
				break;
			case 1:
				placed.randomTableIndex = r.i32();
				placed.randomPositionIndex = r.i32();
				break;
			case 2: {
				const quint32 randomCount = r.u32();
				for (quint32 random = 0; random < randomCount && r.ok(); ++random) {
					RandomTableItem item;
					item.id = r.fourcc();
					item.chancePercent = r.i32();
					placed.randomUnits.push_back(item);
				}
				break;
			}
			default:
				status.error = QStringLiteral("unknown random unit type %1").arg(placed.randomType);
				return status;
		}
		placed.waygateCustomTeamColor = r.i32();
		placed.waygateDestinationRegionIndex = r.i32();
		placed.creationNumber = r.u32();
		if (placedUnits) {
			placedUnits->push_back(placed);
		}
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated units/items");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 subversion=%2 records=%3 itemSets=%4 inventoryItems=%5 modifiedAbilities=%6")
		.arg(version)
		.arg(subversion)
		.arg(count)
		.arg(itemSetCount)
		.arg(inventoryItems)
		.arg(modifiedAbilities);
	return status;
}

QString objectValueToString(quint32 type, BinaryReader& r) {
	switch (type) {
		case 0: return QString::number(r.i32());
		case 1:
		case 2: return QString::number(static_cast<double>(r.f32()), 'g', 8);
		case 3: return r.cstring();
		default: return {};
	}
}

QString readObjectId(BinaryReader& r) {
	const QString id = r.fourcc();
	return id == QStringLiteral("\0\0\0\0") ? QString() : id;
}

FileLoadStatus parseModificationFile(
	const QByteArray& bytes,
	bool optionalInts,
	const QString& fileName,
	ObjectModificationSummary* summary,
	QVector<ObjectModificationRecord>* records
) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const quint32 version = r.u32();
	if (summary) {
		summary->version = version;
	}
	auto readTable = [&](bool customTable, const QString& name, quint32* objectCount, quint32* modificationCount) {
		*objectCount = r.u32();
		for (quint32 i = 0; i < *objectCount && r.ok(); ++i) {
			ObjectModificationRecord record;
			record.fileName = fileName;
			record.customTable = customTable;
			record.originalId = readObjectId(r);
			record.newId = readObjectId(r);
			if (version >= 3) {
				r.skip(8);
			}
			const quint32 mods = r.u32();
			*modificationCount += mods;
			for (quint32 j = 0; j < mods && r.ok(); ++j) {
				ObjectModificationValue mod;
				mod.fieldId = r.fourcc();
				mod.type = r.u32();
				if (optionalInts) {
					mod.level = r.u32();
					mod.dataPointer = r.u32();
					mod.variation = mod.level;
					mod.abilityDataColumn = mod.dataPointer;
				}
				if (mod.type > 3) {
					status.error = QStringLiteral("unknown modification type %1 in %2").arg(mod.type).arg(name);
					return;
				}
				mod.value = objectValueToString(mod.type, r);
				mod.parentObjectId = readObjectId(r);
				record.modifications.push_back(mod);
			}
			if (records) {
				records->push_back(record);
			}
		}
	};
	quint32 originalObjects = 0;
	quint32 customObjects = 0;
	quint32 originalMods = 0;
	quint32 customMods = 0;
	readTable(false, QStringLiteral("original"), &originalObjects, &originalMods);
	readTable(true, QStringLiteral("custom"), &customObjects, &customMods);
	if (!status.error.isEmpty()) {
		return status;
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated modification file");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 originalObjects=%2 originalMods=%3 customObjects=%4 customMods=%5")
		.arg(version)
		.arg(originalObjects)
		.arg(originalMods)
		.arg(customObjects)
		.arg(customMods);
	if (summary) {
		summary->parsed = true;
		summary->originalObjects = originalObjects;
		summary->originalMods = originalMods;
		summary->customObjects = customObjects;
		summary->customMods = customMods;
	}
	return status;
}

FileLoadStatus parseRegions(const QByteArray& bytes, const W3eHeaderInfo& terrain, QVector<RegionInfo>* regions) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const quint32 version = r.u32();
	const quint32 count = r.u32();
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		RegionInfo region;
		const float left = r.f32();
		const float bottom = r.f32();
		const float right = r.f32();
		const float top = r.f32();
		region.bounds = QRectF(QPointF(left, bottom), QPointF(right, top)).normalized();
		region.name = r.cstring();
		region.index = r.i32();
		region.weatherEffectId = r.fourcc();
		region.ambientSoundVariable = r.cstring();
		region.color = readBgraPadded(r);
		if (regions) {
			regions->push_back(region);
		}
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated regions");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 regions=%2 terrainOffset=%3,%4")
		.arg(version)
		.arg(count)
		.arg(terrain.offset.x())
		.arg(terrain.offset.y());
	return status;
}

FileLoadStatus parseCameras(const QByteArray& bytes, const W3iInfo& info, QVector<CameraInfo>* cameras) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const quint32 version = r.u32();
	const quint32 count = r.u32();
	const bool hasLocalRotation = info.gameVersionMajor * 100 + info.gameVersionMinor >= 131;
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		CameraInfo camera;
		camera.target = QPointF(r.f32(), r.f32());
		camera.offsetZ = r.f32();
		camera.rotation = r.f32();
		camera.angleOfAttack = r.f32();
		camera.distance = r.f32();
		camera.roll = r.f32();
		camera.fieldOfView = r.f32();
		camera.farClipping = r.f32();
		camera.unknown = r.f32();
		if (hasLocalRotation) {
			r.skip(12);
		}
		camera.name = r.cstring();
		if (cameras) {
			cameras->push_back(camera);
		}
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated cameras");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 cameras=%2 localRotation=%3")
		.arg(version)
		.arg(count)
		.arg(hasLocalRotation ? QStringLiteral("yes") : QStringLiteral("no"));
	return status;
}

FileLoadStatus parseSounds(const QByteArray& bytes, QVector<SoundInfo>* sounds) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const quint32 version = r.u32();
	const quint32 count = r.u32();
	quint32 looping = 0;
	quint32 sound3d = 0;
	quint32 music = 0;
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		SoundInfo sound;
		sound.variable = r.cstring();
		sound.filePath = r.cstring();
		sound.eaxEffect = r.cstring();
		sound.flags = r.u32();
		if (sound.flags & 0b00000001) ++looping;
		if (sound.flags & 0b00000010) ++sound3d;
		if (sound.flags & 0b00001000) ++music;
		sound.fadeInRate = r.i32();
		sound.fadeOutRate = r.i32();
		sound.volume = r.i32();
		sound.pitch = r.f32();
		sound.unknown1 = r.f32();
		sound.unknown2 = r.i32();
		sound.channel = r.i32();
		sound.distanceMin = r.f32();
		sound.distanceMax = r.f32();
		sound.distanceCutoff = r.f32();
		sound.unknown3 = r.f32();
		sound.unknown4 = r.f32();
		sound.unknown5 = r.i32();
		sound.unknown6 = r.f32();
		sound.unknown7 = r.f32();
		sound.unknown8 = r.f32();
		if (version >= 2) {
			r.skipCString();
			r.skipCString();
			r.skipCString();
			r.skip(4);
			r.skipCString();
			r.skip(4);
			r.skipCString();
			r.skip(4);
			r.skipCString();
			r.skipCString();
			r.skipCString();
			r.skipCString();
			if (version >= 3) {
				r.skip(4);
			}
		}
		if (sounds) {
			sounds->push_back(sound);
		}
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated sounds");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 sounds=%2 looping=%3 3d=%4 music=%5")
		.arg(version)
		.arg(count)
		.arg(looping)
		.arg(sound3d)
		.arg(music);
	return status;
}

FileLoadStatus parseImports(const QByteArray& bytes, QVector<ImportInfo>* imports) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const quint32 version = r.u32();
	const quint32 count = r.u32();
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		ImportInfo import;
		const quint8 flag = r.u8();
		import.customPath = flag == 10 || flag == 13;
		import.path = r.cstring();
		if (imports) imports->push_back(import);
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated imports");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 imports=%2").arg(version).arg(count);
	return status;
}

FileLoadStatus parseMinimapIcons(const QByteArray& bytes, QVector<MinimapIconInfo>* icons) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	const quint32 version = r.u32();
	const quint32 count = r.u32();
	for (quint32 i = 0; i < count && r.ok(); ++i) {
		MinimapIconInfo icon;
		icon.type = r.i32();
		icon.x = r.i32();
		icon.y = r.i32();
		const int b = r.u8();
		const int g = r.u8();
		const int red = r.u8();
		const int a = r.u8();
		icon.color = QColor(red, g, b, a);
		if (icons) icons->push_back(icon);
	}
	if (!r.ok()) {
		status.error = QStringLiteral("truncated minimap icons");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 icons=%2").arg(version).arg(count);
	return status;
}

FileLoadStatus parseShadowMap(const QByteArray& bytes, ShadowMapInfo* shadowMap) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	quint32 shadows = 0;
	if (shadowMap) {
		shadowMap->valid = true;
		shadowMap->shadows.reserve(bytes.size());
	}
	for (char byte : bytes) {
		const bool shadow = static_cast<quint8>(byte) == 0x00;
		if (shadow) ++shadows;
		if (shadowMap) shadowMap->shadows.push_back(shadow);
	}
	status.parsed = true;
	status.summary = QStringLiteral("bytes=%1 shadows=%2").arg(bytes.size()).arg(shadows);
	return status;
}

FileLoadStatus parseCountedFile(const QByteArray& bytes, const QString& expectedMagic, const QString& label) {
	FileLoadStatus status;
	status.present = true;
	status.bytes = bytes.size();
	BinaryReader r(bytes);
	if (!expectedMagic.isEmpty()) {
		const QString magic = r.fourcc();
		if (magic != expectedMagic) {
			status.error = QStringLiteral("invalid signature: %1").arg(magic);
			return status;
		}
	}
	const quint32 version = r.u32();
	const quint32 count = r.u32();
	if (!r.ok()) {
		status.error = QStringLiteral("truncated header");
		return status;
	}
	status.parsed = true;
	status.summary = QStringLiteral("version=%1 %2=%3").arg(version).arg(label).arg(count);
	return status;
}

FileLoadStatus rawPresent(const QByteArray& bytes, const QString& label) {
	FileLoadStatus status;
	status.present = true;
	status.parsed = true;
	status.bytes = bytes.size();
	status.summary = QStringLiteral("%1 bytes=%2").arg(label).arg(bytes.size());
	return status;
}

void logFile(MapLoadResult* result, const QString& stage, const QString& detail) {
	result->log.push_back(QStringLiteral("%1: %2").arg(stage, detail));
}

void addSlkRecordsToDatabase(
	const GameDataLoadResult& gameData,
	const QString& fileName,
	const QString& category,
	QHash<QString, ObjectDatabaseRecord>* target
) {
	const auto it = gameData.files.find(fileName);
	if (it == gameData.files.end() || !it->loaded) {
		return;
	}

	for (auto recordIt = it->records.cbegin(); recordIt != it->records.cend(); ++recordIt) {
		ObjectDatabaseRecord record;
		record.category = category;
		record.id = recordIt.key();
		record.baseId = record.id;
		record.fields = recordIt.value();
		target->insert(record.id, record);
	}
}

QHash<QString, ObjectDatabaseRecord>* databaseTableForFile(ObjectDatabase* database, const QString& fileName) {
	if (fileName.endsWith(QStringLiteral(".w3u"))) return &database->units;
	if (fileName.endsWith(QStringLiteral(".w3t"))) return &database->items;
	if (fileName.endsWith(QStringLiteral(".w3b"))) return &database->destructables;
	if (fileName.endsWith(QStringLiteral(".w3d"))) return &database->doodads;
	if (fileName.endsWith(QStringLiteral(".w3a"))) return &database->abilities;
	if (fileName.endsWith(QStringLiteral(".w3h"))) return &database->buffs;
	if (fileName.endsWith(QStringLiteral(".w3q"))) return &database->upgrades;
	return nullptr;
}

QString categoryForFile(const QString& fileName) {
	if (fileName.endsWith(QStringLiteral(".w3t"))) return QStringLiteral("item");
	if (fileName.endsWith(QStringLiteral(".w3u"))) return QStringLiteral("unit");
	if (fileName.endsWith(QStringLiteral(".w3b"))) return QStringLiteral("destructable");
	if (fileName.endsWith(QStringLiteral(".w3d"))) return QStringLiteral("doodad");
	if (fileName.endsWith(QStringLiteral(".w3a"))) return QStringLiteral("ability");
	if (fileName.endsWith(QStringLiteral(".w3h"))) return QStringLiteral("buff");
	if (fileName.endsWith(QStringLiteral(".w3q"))) return QStringLiteral("upgrade");
	return QStringLiteral("object");
}

QString firstNonEmpty(const GameDataRecord& record, const QStringList& keys) {
	for (const QString& key : keys) {
		const QString value = record.value(key).trimmed();
		if (!value.isEmpty()) {
			return value;
		}
	}
	return QString();
}

QString normalizeTileKey(const QString& value) {
	return value.trimmed().toLower();
}

QString pickTexturePath(const GameDataRecord& row) {
	QString fullPath = firstNonEmpty(row, {
		QStringLiteral("tex"), QStringLiteral("Tex"),
		QStringLiteral("texture"), QStringLiteral("Texture"),
		QStringLiteral("texPath"), QStringLiteral("texpath"),
		QStringLiteral("groundTile"), QStringLiteral("groundtile")
	});
	if (!fullPath.isEmpty()) {
		return fullPath;
	}

	const QString dir = firstNonEmpty(row, {
		QStringLiteral("dir"), QStringLiteral("Dir"),
		QStringLiteral("texDir"), QStringLiteral("texdir")
	});
	const QString file = firstNonEmpty(row, {
		QStringLiteral("file"), QStringLiteral("File"),
		QStringLiteral("texFile"), QStringLiteral("texfile")
	});
	if (!dir.isEmpty() && !file.isEmpty()) return dir + QStringLiteral("/") + file;
	if (!file.isEmpty()) return file;
	return QString();
}

void buildTerrainVisualDatabase(MapLoadResult* result) {
	const auto terrainIt = result->gameData.files.find(QStringLiteral("TerrainArt/Terrain.slk"));
	if (terrainIt != result->gameData.files.end() && terrainIt->loaded) {
		for (auto it = terrainIt->records.cbegin(); it != terrainIt->records.cend(); ++it) {
			const GameDataRecord& row = it.value();
			TerrainTileTextureInfo info;
			info.tileId = firstNonEmpty(row, {QStringLiteral("tileID"), QStringLiteral("tileid"), QStringLiteral("ID"), QStringLiteral("id")});
			if (info.tileId.isEmpty()) {
				info.tileId = it.key();
			}
			info.displayName = firstNonEmpty(row, {QStringLiteral("comment"), QStringLiteral("name"), QStringLiteral("Name")});
			info.dir = firstNonEmpty(row, {QStringLiteral("dir"), QStringLiteral("Dir"), QStringLiteral("texDir"), QStringLiteral("texdir")});
			info.file = firstNonEmpty(row, {QStringLiteral("file"), QStringLiteral("File"), QStringLiteral("texFile"), QStringLiteral("texfile")});
			info.fullPath = pickTexturePath(row);
			result->terrainVisuals.groundTiles.insert(info.tileId, info);
			result->terrainVisuals.groundTiles.insert(normalizeTileKey(info.tileId), info);
		}
	}

	const auto cliffIt = result->gameData.files.find(QStringLiteral("TerrainArt/CliffTypes.slk"));
	if (cliffIt != result->gameData.files.end() && cliffIt->loaded) {
		for (auto it = cliffIt->records.cbegin(); it != cliffIt->records.cend(); ++it) {
			const GameDataRecord& row = it.value();
			TerrainTileTextureInfo info;
			info.tileId = firstNonEmpty(row, {QStringLiteral("cliffID"), QStringLiteral("cliffid"), QStringLiteral("groundTile"), QStringLiteral("groundtile"), QStringLiteral("ID"), QStringLiteral("id")});
			if (info.tileId.isEmpty()) {
				info.tileId = it.key();
			}
			info.displayName = firstNonEmpty(row, {QStringLiteral("comment"), QStringLiteral("name"), QStringLiteral("Name")});
			info.dir = firstNonEmpty(row, {QStringLiteral("texDir"), QStringLiteral("texdir"), QStringLiteral("dir"), QStringLiteral("Dir")});
			info.file = firstNonEmpty(row, {QStringLiteral("texFile"), QStringLiteral("texfile"), QStringLiteral("file"), QStringLiteral("File")});
			info.fullPath = pickTexturePath(row);
			result->terrainVisuals.cliffTiles.insert(info.tileId, info);
			result->terrainVisuals.cliffTiles.insert(normalizeTileKey(info.tileId), info);
		}
	}

	const auto waterIt = result->gameData.files.find(QStringLiteral("TerrainArt/Water.slk"));
	if (waterIt != result->gameData.files.end() && waterIt->loaded) {
		result->terrainVisuals.waterProfiles = waterIt->records;
	}
}

void bindTerrainTextures(MapLoadResult* result) {
	result->terrainModel.groundTexturePaths.clear();
	result->terrainModel.cliffTexturePaths.clear();
	if (!result->terrainModel.valid) {
		return;
	}

	result->terrainModel.groundTexturePaths.reserve(result->terrainModel.groundTileIds.size());
	for (const QString& tileId : result->terrainModel.groundTileIds) {
		const auto it = result->terrainVisuals.groundTiles.find(tileId);
		if (it != result->terrainVisuals.groundTiles.end()) {
			result->terrainModel.groundTexturePaths.push_back(it->fullPath);
			continue;
		}
		const auto lowerIt = result->terrainVisuals.groundTiles.find(normalizeTileKey(tileId));
		result->terrainModel.groundTexturePaths.push_back(lowerIt == result->terrainVisuals.groundTiles.end() ? QString() : lowerIt->fullPath);
	}

	result->terrainModel.cliffTexturePaths.reserve(result->terrainModel.cliffTileIds.size());
	for (const QString& tileId : result->terrainModel.cliffTileIds) {
		const auto it = result->terrainVisuals.cliffTiles.find(tileId);
		if (it != result->terrainVisuals.cliffTiles.end()) {
			result->terrainModel.cliffTexturePaths.push_back(it->fullPath);
			continue;
		}
		const auto lowerIt = result->terrainVisuals.cliffTiles.find(normalizeTileKey(tileId));
		result->terrainModel.cliffTexturePaths.push_back(lowerIt == result->terrainVisuals.cliffTiles.end() ? QString() : lowerIt->fullPath);
	}

	for (TerrainTileState& tile : result->terrainModel.tiles) {
		tile.groundTileId.clear();
		tile.cliffTileId.clear();
		tile.groundTexturePath.clear();
		tile.cliffTexturePath.clear();

		const int groundIndex = static_cast<int>(tile.groundTexture);
		if (groundIndex >= 0 && groundIndex < result->terrainModel.groundTileIds.size()) {
			tile.groundTileId = result->terrainModel.groundTileIds[groundIndex];
		}
		if (groundIndex >= 0 && groundIndex < result->terrainModel.groundTexturePaths.size()) {
			tile.groundTexturePath = result->terrainModel.groundTexturePaths[groundIndex];
		}

		const int cliffIndex = static_cast<int>(tile.cliffTexture);
		if (cliffIndex >= 0 && cliffIndex < result->terrainModel.cliffTileIds.size()) {
			tile.cliffTileId = result->terrainModel.cliffTileIds[cliffIndex];
		}
		if (cliffIndex >= 0 && cliffIndex < result->terrainModel.cliffTexturePaths.size()) {
			tile.cliffTexturePath = result->terrainModel.cliffTexturePaths[cliffIndex];
		}
	}
}

void buildTerrainDrawBatches(MapLoadResult* result) {
	result->terrainModel.drawBatches = TerrainDrawBatches{};
	if (!result->terrainModel.valid || result->terrainModel.tiles.isEmpty()) {
		return;
	}

	QHash<QString, int> groundBatchIndexByTexture;
	QHash<QString, int> cliffBatchIndexByTexture;

	for (int i = 0; i < result->terrainModel.tiles.size(); ++i) {
		const TerrainTileState& tile = result->terrainModel.tiles[i];

		if (tile.groundTexturePath.isEmpty()) {
			result->terrainModel.drawBatches.unresolvedGroundTiles.push_back(static_cast<quint32>(i));
		} else {
			const auto it = groundBatchIndexByTexture.find(tile.groundTexturePath);
			if (it == groundBatchIndexByTexture.end()) {
				TerrainMaterialBatch batch;
				batch.texturePath = tile.groundTexturePath;
				batch.tileIndices.push_back(static_cast<quint32>(i));
				const int index = result->terrainModel.drawBatches.ground.size();
				result->terrainModel.drawBatches.ground.push_back(batch);
				groundBatchIndexByTexture.insert(tile.groundTexturePath, index);
			} else {
				result->terrainModel.drawBatches.ground[*it].tileIndices.push_back(static_cast<quint32>(i));
			}
		}

		if (tile.cliffTexturePath.isEmpty()) {
			const bool hasCliffTileId = !tile.cliffTileId.isEmpty();
			const bool hasCliffGeometryHint = tile.layerHeight != 2 || tile.ramp;
			if (hasCliffTileId && hasCliffGeometryHint) {
				result->terrainModel.drawBatches.unresolvedCliffTiles.push_back(static_cast<quint32>(i));
			}
		} else {
			const auto it = cliffBatchIndexByTexture.find(tile.cliffTexturePath);
			if (it == cliffBatchIndexByTexture.end()) {
				TerrainMaterialBatch batch;
				batch.texturePath = tile.cliffTexturePath;
				batch.tileIndices.push_back(static_cast<quint32>(i));
				const int index = result->terrainModel.drawBatches.cliff.size();
				result->terrainModel.drawBatches.cliff.push_back(batch);
				cliffBatchIndexByTexture.insert(tile.cliffTexturePath, index);
			} else {
				result->terrainModel.drawBatches.cliff[*it].tileIndices.push_back(static_cast<quint32>(i));
			}
		}
	}
}

void applyObjectModifications(MapLoadResult* result) {
	for (const ObjectModificationRecord& modificationRecord : result->objectModificationRecords) {
		QHash<QString, ObjectDatabaseRecord>* table = databaseTableForFile(&result->objectDatabase, modificationRecord.fileName);
		if (!table) {
			continue;
		}

		const QString targetId = modificationRecord.customTable && !modificationRecord.newId.isEmpty()
			? modificationRecord.newId
			: modificationRecord.originalId;
		if (targetId.isEmpty()) {
			continue;
		}

		ObjectDatabaseRecord object = table->value(targetId);
		if (object.id.isEmpty() && !modificationRecord.originalId.isEmpty()) {
			object = table->value(modificationRecord.originalId);
		}
		if (object.id.isEmpty()) {
			object.category = categoryForFile(modificationRecord.fileName);
			object.baseId = modificationRecord.originalId;
		}

		object.id = targetId;
		object.custom = object.custom || modificationRecord.customTable;
		for (const ObjectModificationValue& modification : modificationRecord.modifications) {
			object.fields.insert(modification.fieldId, modification.value);
		}
		table->insert(object.id, object);
	}
}

void buildObjectDatabase(MapLoadResult* result) {
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Units/UnitData.slk"), QStringLiteral("unit"), &result->objectDatabase.units);
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Units/ItemData.slk"), QStringLiteral("item"), &result->objectDatabase.items);
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Units/DestructableData.slk"), QStringLiteral("destructable"), &result->objectDatabase.destructables);
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Doodads/Doodads.slk"), QStringLiteral("doodad"), &result->objectDatabase.doodads);
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Units/AbilityData.slk"), QStringLiteral("ability"), &result->objectDatabase.abilities);
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Units/AbilityBuffData.slk"), QStringLiteral("buff"), &result->objectDatabase.buffs);
	addSlkRecordsToDatabase(result->gameData, QStringLiteral("Units/UpgradeData.slk"), QStringLiteral("upgrade"), &result->objectDatabase.upgrades);
	applyObjectModifications(result);
}
}

MapLoadResult MapLoader::load(const QString& path) {
	MapLoadResult result;
	result.inputPath = path;

	MapFileSource source;
	if (!source.open(path, &result.error)) {
		result.log.push_back(QStringLiteral("open: failed - %1").arg(result.error));
		return result;
	}

	result.hierarchy.mapDirectory = source.mapDirectory();
	result.hierarchy.archiveBacked = source.isArchive();
	logFile(&result, QStringLiteral("hierarchy.map_directory"), result.hierarchy.mapDirectory);
	if (source.isArchive()) {
		result.archive = parseMapArchiveInfo(source.archivePath());
		logFile(&result, QStringLiteral("map archive"), QStringLiteral("header=%1 footer=%2 name=\"%3\" flags=%4 players=%5")
			.arg(result.archive.validHeader ? QStringLiteral("HM3W") : QStringLiteral("missing"))
			.arg(result.archive.validFooter ? QStringLiteral("NGIS") : QStringLiteral("missing"))
			.arg(result.archive.name)
			.arg(result.archive.flags)
			.arg(result.archive.playersCount));
	}

	logFile(&result, QStringLiteral("trigger strings / triggers"), QStringLiteral("reserved; intentionally skipped for now"));

	QByteArray bytes;
	QString error;
	if (source.readFile(QStringLiteral("war3mapMisc.txt"), &bytes, &error)) {
		result.files.insert(QStringLiteral("war3mapMisc.txt"), rawPresent(bytes, QStringLiteral("gameplay constants")));
		logFile(&result, QStringLiteral("gameplay constants"), QStringLiteral("war3mapMisc.txt bytes=%1").arg(bytes.size()));
	} else {
		logFile(&result, QStringLiteral("gameplay constants"), QStringLiteral("no map override file; defaults will come from game data later"));
	}

	if (source.readFile(QStringLiteral("war3map.w3i"), &bytes, &error)) {
		result.info = parseW3i(bytes, &result.metadata);
		FileLoadStatus st = rawPresent(bytes, QStringLiteral("map info"));
		st.parsed = result.info.valid;
		st.error = result.info.error;
		st.summary = result.info.valid
			? QStringLiteral("version=%1 mapVersion=%2 editor=%3 game=%4.%5 players=%6 forces=%7 name=\"%8\" author=\"%9\"")
				.arg(result.info.version)
				.arg(result.info.mapVersion)
				.arg(result.info.editorVersion)
				.arg(result.info.gameVersionMajor)
				.arg(result.info.gameVersionMinor)
				.arg(result.info.players)
				.arg(result.info.forces)
				.arg(result.info.name, result.info.author)
			: result.info.error;
		result.files.insert(QStringLiteral("war3map.w3i"), st);
		logFile(&result, QStringLiteral("war3map.w3i"), st.summary);
	} else {
		logFile(&result, QStringLiteral("war3map.w3i"), error);
	}

	if (source.readFile(QStringLiteral("war3map.w3e"), &bytes, &error)) {
		result.terrain = W3eLoader::parseW3eFromBytes(bytes, source.sourcePathFor(QStringLiteral("war3map.w3e")));
		logFile(&result, QStringLiteral("war3map.w3e terrain"), result.terrain.valid
			? QStringLiteral("version=%1 size=%2x%3 corners=%4").arg(result.terrain.version).arg(result.terrain.width).arg(result.terrain.height).arg(result.terrain.corners.size())
			: result.terrain.error);
	} else {
		result.terrain.error = error;
		logFile(&result, QStringLiteral("war3map.w3e terrain"), error);
	}

	result.gameData = GameDataLoader::load(result.hierarchy.mapDirectory, result.terrain.tileset);
	for (const QString& line : result.gameData.log) {
		logFile(&result, QStringLiteral("SLK/TXT/INI game data"), line);
	}
	if (!result.gameData.files.isEmpty()) {
		const QStringList sampleFiles = {
			QStringLiteral("Units/UnitData.slk"),
			QStringLiteral("Units/UnitMetaData.slk"),
			QStringLiteral("Doodads/Doodads.slk"),
			QStringLiteral("Units/AbilityData.slk"),
			QStringLiteral("Units/MiscData.txt")
		};
		for (const QString& fileName : sampleFiles) {
			const auto it = result.gameData.files.find(fileName);
			if (it == result.gameData.files.end()) continue;
			const GameDataFileInfo& info = it.value();
			logFile(&result, QStringLiteral("game data file"), info.loaded
				? QStringLiteral("%1 bytes=%2 rows=%3 sections=%4 samples=%5 source=%6")
					.arg(fileName)
					.arg(info.bytes)
					.arg(info.rows)
					.arg(info.sections)
					.arg(info.sampleKeys.join(QStringLiteral(", ")))
					.arg(info.source)
				: QStringLiteral("%1 missing").arg(fileName));
		}
	}

	if (source.readFile(QStringLiteral("war3map.wpm"), &bytes, &error)) {
		result.pathing = parseWpm(bytes);
		FileLoadStatus st = rawPresent(bytes, QStringLiteral("pathing"));
		st.parsed = result.pathing.valid;
		st.error = result.pathing.error;
		st.summary = result.pathing.valid
			? QStringLiteral("version=%1 size=%2x%3 cells=%4 canWalk=%5 canBuild=%6 ground=%7")
				.arg(result.pathing.version)
				.arg(result.pathing.width)
				.arg(result.pathing.height)
				.arg(result.pathing.cells)
				.arg(result.pathing.canWalk)
				.arg(result.pathing.canBuild)
				.arg(result.pathing.ground)
			: result.pathing.error;
		result.files.insert(QStringLiteral("war3map.wpm"), st);
		logFile(&result, QStringLiteral("war3map.wpm pathing"), st.summary);
	} else {
		logFile(&result, QStringLiteral("war3map.wpm pathing"), result.terrain.valid
			? QStringLiteral("missing; fallback grid %1x%2").arg(result.terrain.width * 4).arg(result.terrain.height * 4)
			: error);
	}

	const QStringList objectFiles = {
		QStringLiteral("war3map.w3d"), QStringLiteral("war3mapSkin.w3d"),
		QStringLiteral("war3map.w3b"), QStringLiteral("war3mapSkin.w3b"),
		QStringLiteral("war3map.w3u"), QStringLiteral("war3mapSkin.w3u"),
		QStringLiteral("war3map.w3t"), QStringLiteral("war3mapSkin.w3t"),
		QStringLiteral("war3map.w3a"), QStringLiteral("war3mapSkin.w3a"),
		QStringLiteral("war3map.w3h"), QStringLiteral("war3mapSkin.w3h"),
		QStringLiteral("war3map.w3q"), QStringLiteral("war3mapSkin.w3q")
	};
	int presentObjectFiles = 0;
	for (const QString& fileName : objectFiles) {
		ObjectModificationSummary summary;
		summary.fileName = fileName;
		if (source.readFile(fileName, &bytes)) {
			summary.present = true;
			summary.bytes = bytes.size();
			++presentObjectFiles;
			const bool optionalInts =
				fileName.endsWith(QStringLiteral(".w3d")) ||
				fileName.endsWith(QStringLiteral(".w3a")) ||
				fileName.endsWith(QStringLiteral(".w3q"));
			FileLoadStatus st = parseModificationFile(bytes, optionalInts, fileName, &summary, &result.objectModificationRecords);
			result.files.insert(fileName, st);
			logFile(&result, QStringLiteral("object modification"), QStringLiteral("%1: %2")
				.arg(fileName, st.parsed ? st.summary : st.error));
		}
		result.objectModifications.push_back(summary);
	}
	logFile(&result, QStringLiteral("object modification files"), QStringLiteral("%1/%2 present").arg(presentObjectFiles).arg(objectFiles.size()));

	buildObjectDatabase(&result);
	logFile(&result, QStringLiteral("object database"), QStringLiteral("units=%1 items=%2 destructables=%3 doodads=%4 abilities=%5 buffs=%6 upgrades=%7 mapMods=%8")
		.arg(result.objectDatabase.units.size())
		.arg(result.objectDatabase.items.size())
		.arg(result.objectDatabase.destructables.size())
		.arg(result.objectDatabase.doodads.size())
		.arg(result.objectDatabase.abilities.size())
		.arg(result.objectDatabase.buffs.size())
		.arg(result.objectDatabase.upgrades.size())
		.arg(result.objectModificationRecords.size()));

	buildTerrainVisualDatabase(&result);
	logFile(&result, QStringLiteral("terrain visuals"), QStringLiteral("ground=%1 cliff=%2 waterProfiles=%3")
		.arg(result.terrainVisuals.groundTiles.size())
		.arg(result.terrainVisuals.cliffTiles.size())
		.arg(result.terrainVisuals.waterProfiles.size()));

	if (source.readFile(QStringLiteral("war3map.doo"), &bytes, &error)) {
		const FileLoadStatus st = parseDoodads(bytes, result.info.gameVersionMajor, result.info.gameVersionMinor, &result.placedDoodads, &result.terrainDoodads);
		result.files.insert(QStringLiteral("war3map.doo"), st);
		logFile(&result, QStringLiteral("doodads"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("doodads"), error);
	}

	if (source.readFile(QStringLiteral("war3mapUnits.doo"), &bytes, &error)) {
		const FileLoadStatus st = parseUnits(bytes, result.info.gameVersionMajor, result.info.gameVersionMinor, &result.placedUnits);
		result.files.insert(QStringLiteral("war3mapUnits.doo"), st);
		logFile(&result, QStringLiteral("units/items"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("units/items"), error);
	}

	if (source.readFile(QStringLiteral("war3map.w3r"), &bytes, &error)) {
		const FileLoadStatus st = parseRegions(bytes, result.terrain, &result.regions);
		result.files.insert(QStringLiteral("war3map.w3r"), st);
		logFile(&result, QStringLiteral("regions"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("regions"), error);
	}

	if (source.readFile(QStringLiteral("war3map.w3c"), &bytes, &error)) {
		const FileLoadStatus st = parseCameras(bytes, result.info, &result.cameras);
		result.files.insert(QStringLiteral("war3map.w3c"), st);
		logFile(&result, QStringLiteral("cameras"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("cameras"), error);
	}

	if (source.readFile(QStringLiteral("war3map.w3s"), &bytes, &error)) {
		const FileLoadStatus st = parseSounds(bytes, &result.sounds);
		result.files.insert(QStringLiteral("war3map.w3s"), st);
		logFile(&result, QStringLiteral("sounds"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("sounds"), error);
	}

	if (source.readFile(QStringLiteral("war3map.imp"), &bytes, &error)) {
		const FileLoadStatus st = parseImports(bytes, &result.imports);
		result.files.insert(QStringLiteral("war3map.imp"), st);
		logFile(&result, QStringLiteral("imports"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("imports"), error);
	}

	if (source.readFile(QStringLiteral("war3map.mmp"), &bytes, &error)) {
		const FileLoadStatus st = parseMinimapIcons(bytes, &result.minimapIcons);
		result.files.insert(QStringLiteral("war3map.mmp"), st);
		logFile(&result, QStringLiteral("minimap icons"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("minimap icons"), error);
	}

	if (source.readFile(QStringLiteral("war3map.shd"), &bytes, &error)) {
		const FileLoadStatus st = parseShadowMap(bytes, &result.shadowMap);
		result.files.insert(QStringLiteral("war3map.shd"), st);
		logFile(&result, QStringLiteral("shadow map"), st.parsed ? st.summary : st.error);
	} else {
		logFile(&result, QStringLiteral("shadow map"), error);
	}

	result.terrainModel = TerrainModelBuilder::build(result.terrain, result.pathing, result.shadowMap);
	bindTerrainTextures(&result);
	buildTerrainDrawBatches(&result);
	logFile(&result, QStringLiteral("terrain model"), result.terrainModel.valid
		? QStringLiteral("corners=%1 tiles=%2 meshVertices=%3 meshTriangles=%4 pathing=%5 shadows=%6")
			.arg(result.terrainModel.corners.size())
			.arg(result.terrainModel.tiles.size())
			.arg(result.terrainModel.mesh.vertices.size())
			.arg(result.terrainModel.mesh.indices.size() / 3)
			.arg(result.terrainModel.pathing.size())
			.arg(result.terrainModel.shadows.size())
		: result.terrainModel.error);
	if (result.terrainModel.valid) {
		int missingGround = 0;
		QStringList missingGroundIds;
		for (int i = 0; i < result.terrainModel.groundTexturePaths.size(); ++i) {
			const QString& path = result.terrainModel.groundTexturePaths[i];
			if (!path.isEmpty()) {
				continue;
			}
			++missingGround;
			if (i >= 0 && i < result.terrainModel.groundTileIds.size()) {
				missingGroundIds.push_back(result.terrainModel.groundTileIds[i]);
			}
		}
		int missingCliff = 0;
		QStringList missingCliffIds;
		for (int i = 0; i < result.terrainModel.cliffTexturePaths.size(); ++i) {
			const QString& path = result.terrainModel.cliffTexturePaths[i];
			if (!path.isEmpty()) {
				continue;
			}
			++missingCliff;
			if (i >= 0 && i < result.terrainModel.cliffTileIds.size()) {
				missingCliffIds.push_back(result.terrainModel.cliffTileIds[i]);
			}
		}
		logFile(&result, QStringLiteral("terrain texture binding"), QStringLiteral("ground=%1 missing=%2 cliff=%3 missing=%4")
			.arg(result.terrainModel.groundTexturePaths.size())
			.arg(missingGround)
			.arg(result.terrainModel.cliffTexturePaths.size())
			.arg(missingCliff));
		if (!result.terrainModel.tiles.isEmpty()) {
			const TerrainTileState& firstTile = result.terrainModel.tiles.front();
			logFile(&result, QStringLiteral("terrain tile sample"), QStringLiteral("ground=%1 path=%2 cliff=%3 path=%4")
				.arg(firstTile.groundTileId)
				.arg(firstTile.groundTexturePath)
				.arg(firstTile.cliffTileId)
				.arg(firstTile.cliffTexturePath));
		}
		logFile(&result, QStringLiteral("terrain draw batches"), QStringLiteral("ground=%1 cliff=%2 unresolvedGround=%3 unresolvedCliff=%4")
			.arg(result.terrainModel.drawBatches.ground.size())
			.arg(result.terrainModel.drawBatches.cliff.size())
			.arg(result.terrainModel.drawBatches.unresolvedGroundTiles.size())
			.arg(result.terrainModel.drawBatches.unresolvedCliffTiles.size()));
		if (!missingGroundIds.isEmpty()) {
			missingGroundIds.removeDuplicates();
			logFile(&result, QStringLiteral("terrain texture missing ground"), missingGroundIds.join(QStringLiteral(", ")));
		}
		if (!missingCliffIds.isEmpty()) {
			missingCliffIds.removeDuplicates();
			logFile(&result, QStringLiteral("terrain texture missing cliff"), missingCliffIds.join(QStringLiteral(", ")));
		}
	}

	result.loaded = result.terrain.valid;
	logFile(&result, QStringLiteral("loaded"), result.loaded ? QStringLiteral("true") : QStringLiteral("false"));
	return result;
}
