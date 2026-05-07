#include "game_data_loader.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSettings>
#include <QStringList>

#if defined(UNICODE)
#define W3EDITOR_RESTORE_UNICODE
#undef UNICODE
#endif
#if defined(_UNICODE)
#define W3EDITOR_RESTORE__UNICODE
#undef _UNICODE
#endif
#include <CascLib.h>
#if defined(W3EDITOR_RESTORE_UNICODE)
#define UNICODE
#undef W3EDITOR_RESTORE_UNICODE
#endif
#if defined(W3EDITOR_RESTORE__UNICODE)
#define _UNICODE
#undef W3EDITOR_RESTORE__UNICODE
#endif

#if defined(UNICODE)
#define W3EDITOR_RESTORE_STORM_UNICODE
#undef UNICODE
#endif
#if defined(_UNICODE)
#define W3EDITOR_RESTORE_STORM__UNICODE
#undef _UNICODE
#endif
#include <StormLib.h>
#if defined(W3EDITOR_RESTORE_STORM_UNICODE)
#define UNICODE
#undef W3EDITOR_RESTORE_STORM_UNICODE
#endif
#if defined(W3EDITOR_RESTORE_STORM__UNICODE)
#define _UNICODE
#undef W3EDITOR_RESTORE_STORM__UNICODE
#endif

namespace {
class CascStorage {
public:
	~CascStorage() {
		if (handle_) {
			CascCloseStorage(handle_);
		}
	}

	bool open(const QString& storagePath, QString* error) {
		const QByteArray bytes = QDir::toNativeSeparators(storagePath).toLocal8Bit();
		if (!CascOpenStorage(bytes.constData(), CASC_LOCALE_ALL, &handle_)) {
			*error = QStringLiteral("CascOpenStorage failed: %1 (GetCascError=%2)")
				.arg(storagePath)
				.arg(static_cast<qulonglong>(GetCascError()));
			return false;
		}
		return true;
	}

	bool read(const QString& fileName, QByteArray* out) const {
		out->clear();
		if (!handle_) return false;

		HANDLE file = nullptr;
		const QByteArray name = fileName.toLocal8Bit();
		if (!CascOpenFile(handle_, name.constData(), 0, CASC_OPEN_BY_NAME, &file)) {
			return false;
		}

		const DWORD size = CascGetFileSize(file, nullptr);
		if (size == CASC_INVALID_SIZE) {
			CascCloseFile(file);
			return false;
		}

		out->resize(static_cast<int>(size));
		DWORD read = 0;
		const bool ok = CascReadFile(file, out->data(), size, &read) && read == size;
		CascCloseFile(file);
		if (!ok) {
			out->clear();
		}
		return ok;
	}

private:
	HANDLE handle_ = nullptr;
};

class MpqChain {
public:
	~MpqChain() {
		if (archive_) {
			SFileCloseArchive(archive_);
		}
	}

	bool open(const QString& warcraftDirectory) {
		const QString basePath = QDir(warcraftDirectory).filePath(QStringLiteral("war3.mpq"));
		if (!QFileInfo::exists(basePath)) {
			return false;
		}

		const QByteArray baseBytes = QDir::toNativeSeparators(basePath).toLocal8Bit();
		if (!SFileOpenArchive(baseBytes.constData(), 0, STREAM_FLAG_READ_ONLY, &archive_)) {
			return false;
		}
		archiveDescription_ = basePath;

		const QStringList patches = {
			QStringLiteral("War3x.mpq"),
			QStringLiteral("War3xLocal.mpq"),
			QStringLiteral("War3Patch.mpq")
		};
		for (const QString& patchName : patches) {
			const QString patchPath = QDir(warcraftDirectory).filePath(patchName);
			if (!QFileInfo::exists(patchPath)) continue;

			const QByteArray patchBytes = QDir::toNativeSeparators(patchPath).toLocal8Bit();
			if (SFileOpenPatchArchive(archive_, patchBytes.constData(), "", 0)) {
				archiveDescription_ += QStringLiteral(" + ") + patchPath;
				++patchCount_;
			}
		}
		return true;
	}

	bool read(const QString& fileName, QByteArray* out, QString* source) const {
		out->clear();
		if (!archive_) return false;

		QStringList candidates;
		const QString native = QDir::toNativeSeparators(fileName);
		const QString forward = QDir::fromNativeSeparators(fileName);
		const QString lowerNative = native.toLower();
		const QString lowerForward = forward.toLower();
		candidates << fileName << native << forward << lowerNative << lowerForward;
		candidates.removeDuplicates();

		for (const QString& candidate : candidates) {
			HANDLE file = nullptr;
			const QByteArray bytes = candidate.toLocal8Bit();
			if (!SFileOpenFileEx(archive_, bytes.constData(), 0, &file)) {
				continue;
			}
			const DWORD size = SFileGetFileSize(file, nullptr);
			if (size == SFILE_INVALID_SIZE) {
				SFileCloseFile(file);
				continue;
			}
			out->resize(static_cast<int>(size));
			DWORD read = 0;
			const bool ok = SFileReadFile(file, out->data(), size, &read, nullptr) && read == size;
			SFileCloseFile(file);
			if (ok) {
				*source = archiveDescription_ + QStringLiteral(":") + candidate;
				return true;
			}
			out->clear();
		}
		return false;
	}

	int archiveCount() const { return archive_ ? 1 + patchCount_ : 0; }

private:
	HANDLE archive_ = nullptr;
	int patchCount_ = 0;
	QString archiveDescription_;
};

bool looksLikeCascWarcraftDirectory(const QString& directory) {
	return QFileInfo::exists(QDir(directory).filePath(QStringLiteral(".build.info"))) ||
		QDir(QDir(directory).filePath(QStringLiteral("_retail_"))).exists();
}

bool looksLikeMpqWarcraftDirectory(const QString& directory) {
	return QFileInfo::exists(QDir(directory).filePath(QStringLiteral("war3.mpq"))) ||
		QFileInfo::exists(QDir(directory).filePath(QStringLiteral("War3x.mpq")));
}

QStringList findWarcraftDirectories() {
	QStringList directories;
	auto addDirectory = [&](const QString& value) {
		if (value.isEmpty()) return;
		const QFileInfo info(value);
		const QString directory = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
		if (!directory.isEmpty() && QDir(directory).exists() && !directories.contains(directory)) {
			directories.push_back(directory);
		}
	};

	addDirectory(QString::fromLocal8Bit(qgetenv("WAR3_DIR")));
	addDirectory(QString::fromLocal8Bit(qgetenv("WARCRAFT_III_DIR")));

	const QStringList registryKeys = {
		QStringLiteral("HKEY_CURRENT_USER\\Software\\Blizzard Entertainment\\Warcraft III"),
		QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\WOW6432Node\\Blizzard Entertainment\\Warcraft III"),
		QStringLiteral("HKEY_LOCAL_MACHINE\\Software\\Blizzard Entertainment\\Warcraft III")
	};
	const QStringList valueNames = {
		QStringLiteral("InstallPath"),
		QStringLiteral("InstallPathX"),
		QStringLiteral("GamePath")
	};
	for (const QString& key : registryKeys) {
		QSettings settings(key, QSettings::NativeFormat);
		for (const QString& valueName : valueNames) {
			addDirectory(settings.value(valueName).toString());
		}
	}

	const QStringList candidates = {
		QStringLiteral("C:/war3/War3"),
		QStringLiteral("C:/war3/WarcraftXM"),
		QStringLiteral("C:/Program Files/Warcraft III"),
		QStringLiteral("C:/Program Files (x86)/Warcraft III"),
		QStringLiteral("D:/Warcraft III"),
		QStringLiteral("D:/Games/Warcraft III")
	};
	for (const QString& candidate : candidates) {
		addDirectory(candidate);
	}
	return directories;
}

QStringList cascCandidates(const QString& warcraftDirectory) {
	return {
		QDir(warcraftDirectory).filePath(QStringLiteral(":w3")),
		QDir(warcraftDirectory).filePath(QStringLiteral("_retail_")),
		warcraftDirectory
	};
}

int countSlkRows(const QByteArray& bytes) {
	const QString text = QString::fromLatin1(bytes);
	QSet<QString> rows;
	const QRegularExpression rowRe(QStringLiteral("Y(\\d+)"));
	for (const QString& line : text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
		if (!line.startsWith(QLatin1Char('C'))) continue;
		const auto match = rowRe.match(line);
		if (match.hasMatch()) {
			const int row = match.captured(1).toInt();
			if (row > 1) {
				rows.insert(match.captured(1));
			}
		}
	}
	return rows.size();
}

int countIniSections(const QByteArray& bytes) {
	const QString text = QString::fromUtf8(bytes);
	int count = 0;
	for (const QString& line : text.split(QRegularExpression(QStringLiteral("[\\r\\n]+")), Qt::SkipEmptyParts)) {
		const QString trimmed = line.trimmed();
		if (trimmed.startsWith(QLatin1Char('[')) && trimmed.contains(QLatin1Char(']'))) {
			++count;
		}
	}
	return count;
}

QStringList splitSlkLine(const QString& line) {
	QStringList parts;
	QString current;
	bool quoted = false;
	for (int i = 0; i < line.size(); ++i) {
		const QChar ch = line.at(i);
		if (ch == QLatin1Char('"')) {
			if (quoted && i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
				current += ch;
				++i;
			} else {
				quoted = !quoted;
			}
		} else if (ch == QLatin1Char(';') && !quoted) {
			parts.push_back(current);
			current.clear();
		} else {
			current += ch;
		}
	}
	parts.push_back(current);
	return parts;
}

void parseSlk(const QByteArray& bytes, GameDataFileInfo* info) {
	const QString text = QString::fromLatin1(bytes);
	QHash<int, QHash<int, QString>> cells;
	int currentX = 0;
	int currentY = 0;

	for (const QString& line : text.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts)) {
		if (!line.startsWith(QLatin1Char('C'))) {
			continue;
		}

		const QStringList parts = splitSlkLine(line);
		QString value;
		for (int i = 1; i < parts.size(); ++i) {
			const QString& part = parts.at(i);
			if (part.startsWith(QLatin1Char('X'))) {
				currentX = part.mid(1).toInt();
			} else if (part.startsWith(QLatin1Char('Y'))) {
				currentY = part.mid(1).toInt();
			} else if (part.startsWith(QLatin1Char('K'))) {
				value = part.mid(1);
			}
		}

		if (currentX > 0 && currentY > 0) {
			cells[currentY][currentX] = value;
		}
	}

	QHash<int, QString> columns;
	for (auto it = cells.value(1).cbegin(); it != cells.value(1).cend(); ++it) {
		columns.insert(it.key(), it.value());
	}

	for (auto rowIt = cells.cbegin(); rowIt != cells.cend(); ++rowIt) {
		const int row = rowIt.key();
		if (row <= 1) {
			continue;
		}

		const QHash<int, QString>& rowCells = rowIt.value();
		const QString id = rowCells.value(1).trimmed();
		if (id.isEmpty()) {
			continue;
		}

		GameDataRecord record;
		for (auto cellIt = rowCells.cbegin(); cellIt != rowCells.cend(); ++cellIt) {
			const int column = cellIt.key();
			const QString field = columns.value(column, column == 1 ? QStringLiteral("ID") : QString::number(column));
			if (!field.isEmpty()) {
				record.insert(field, cellIt.value());
			}
		}
		info->records.insert(id, record);
	}

	info->rows = info->records.size();
	info->sampleKeys = info->records.keys();
	info->sampleKeys.sort();
	if (info->sampleKeys.size() > 5) {
		info->sampleKeys = info->sampleKeys.mid(0, 5);
	}
}

void parseIniLike(const QByteArray& bytes, GameDataFileInfo* info) {
	const QString text = QString::fromLocal8Bit(bytes);
	QString currentSection;

	for (QString line : text.split(QRegularExpression(QStringLiteral("[\r\n]+")))) {
		line = line.trimmed();
		if (line.isEmpty() || line.startsWith(QStringLiteral("//")) || line.startsWith(QLatin1Char('#'))) {
			continue;
		}

		if (line.startsWith(QLatin1Char('['))) {
			const int close = line.indexOf(QLatin1Char(']'));
			if (close > 1) {
				currentSection = line.mid(1, close - 1).trimmed();
				info->sectionValues.insert(currentSection, GameDataRecord{});
			}
			continue;
		}

		const int equals = line.indexOf(QLatin1Char('='));
		if (equals <= 0 || currentSection.isEmpty()) {
			continue;
		}

		const QString key = line.left(equals).trimmed();
		const QString value = line.mid(equals + 1).trimmed();
		if (!key.isEmpty()) {
			info->sectionValues[currentSection].insert(key, value);
		}
	}

	info->sections = info->sectionValues.size();
	info->sampleKeys = info->sectionValues.keys();
	info->sampleKeys.sort();
	if (info->sampleKeys.size() > 5) {
		info->sampleKeys = info->sampleKeys.mid(0, 5);
	}
}

bool readLocalFile(const QString& mapDirectory, const QString& path, QByteArray* out, QString* source) {
	QStringList candidates = {
		QDir(mapDirectory).filePath(path),
		QDir(QStringLiteral("data/overrides")).filePath(path)
	};
	for (const QString& candidate : candidates) {
		QFile file(candidate);
		if (file.open(QIODevice::ReadOnly)) {
			*out = file.readAll();
			*source = candidate;
			return true;
		}
	}
	return false;
}

QStringList gameDataPaths(const QString& path, const QString& tileset) {
	QStringList paths;
	if (!tileset.isEmpty()) {
		paths << QStringLiteral("war3.w3mod:_hd.w3mod:_tilesets/%1.w3mod:%2").arg(tileset, path);
		paths << QStringLiteral("war3.w3mod:_tilesets/%1.w3mod:%2").arg(tileset, path);
	}
	paths << QStringLiteral("war3.w3mod:_hd.w3mod:%1").arg(path);
	paths << QStringLiteral("war3.w3mod:_locales/enus.w3mod:%1").arg(path);
	paths << QStringLiteral("war3.w3mod:%1").arg(path);
	paths << QStringLiteral("war3.w3mod:_deprecated.w3mod:%1").arg(path);
	paths << path;
	return paths;
}

QStringList classicMpqDataPaths(const QString& path) {
	const QString native = QDir::toNativeSeparators(path);
	const QString forward = QDir::fromNativeSeparators(path);
	QStringList paths;
	paths << native;
	if (forward != native) {
		paths << forward;
	}
	return paths;
}

GameDataFileInfo loadOne(
	const QString& mapDirectory,
	const CascStorage* casc,
	const MpqChain* mpq,
	const QString& requestedPath,
	const QString& tileset
) {
	GameDataFileInfo info;
	QByteArray bytes;
	QString source;
	if (readLocalFile(mapDirectory, requestedPath, &bytes, &source)) {
		info.loaded = true;
		info.bytes = bytes.size();
		info.source = source;
	} else if (casc) {
		for (const QString& path : gameDataPaths(requestedPath, tileset)) {
			if (casc->read(path, &bytes)) {
				info.loaded = true;
				info.bytes = bytes.size();
				info.source = path;
				break;
			}
		}
	} else if (mpq) {
		for (const QString& path : classicMpqDataPaths(requestedPath)) {
			if (mpq->read(path, &bytes, &source)) {
				info.loaded = true;
				info.bytes = bytes.size();
				info.source = source;
				break;
			}
		}
	}

	if (!info.loaded) {
		info.error = QStringLiteral("not found");
		return info;
	}

	const QString lower = requestedPath.toLower();
	if (lower.endsWith(QStringLiteral(".slk"))) {
		parseSlk(bytes, &info);
	} else if (lower.endsWith(QStringLiteral(".txt")) || lower.endsWith(QStringLiteral(".ini"))) {
		parseIniLike(bytes, &info);
	}
	return info;
}
}

GameDataLoadResult GameDataLoader::load(const QString& mapDirectory, const QString& tileset) {
	GameDataLoadResult result;
	const QStringList directories = findWarcraftDirectories();
	if (directories.isEmpty()) {
		result.log.push_back(QStringLiteral("game data: Warcraft III install path not found"));
		return result;
	}

	CascStorage storage;
	MpqChain mpq;
	QString error;
	bool mpqOpened = false;
	for (const QString& directory : directories) {
		if (looksLikeMpqWarcraftDirectory(directory) && mpq.open(directory)) {
			result.warcraftDirectory = directory;
			result.storagePath = directory;
			mpqOpened = true;
			break;
		}
	}
	if (!mpqOpened) {
		for (const QString& directory : directories) {
			if (!looksLikeCascWarcraftDirectory(directory)) {
				continue;
			}
			for (const QString& candidate : cascCandidates(directory)) {
				if (storage.open(candidate, &error)) {
					result.warcraftDirectory = directory;
					result.cascOpened = true;
					result.storagePath = candidate;
					break;
				}
			}
			if (result.cascOpened) break;
		}
	}
	if (!result.cascOpened && !mpqOpened) {
		result.log.push_back(QStringLiteral("game data: no usable CASC/MPQ storage found; last CASC error: %1").arg(error));
		return result;
	}

	const QStringList files = {
		QStringLiteral("TerrainArt/Terrain.slk"),
		QStringLiteral("TerrainArt/CliffTypes.slk"),
		QStringLiteral("TerrainArt/Water.slk"),
		QStringLiteral("Units/UnitData.slk"),
		QStringLiteral("Units/UnitMetaData.slk"),
		QStringLiteral("Units/UnitBalance.slk"),
		QStringLiteral("Units/unitUI.slk"),
		QStringLiteral("Units/UnitWeapons.slk"),
		QStringLiteral("Units/UnitAbilities.slk"),
		QStringLiteral("Units/ItemData.slk"),
		QStringLiteral("Units/ItemMetaData.slk"),
		QStringLiteral("Doodads/Doodads.slk"),
		QStringLiteral("Doodads/DoodadMetaData.slk"),
		QStringLiteral("Units/DestructableData.slk"),
		QStringLiteral("Units/DestructableMetaData.slk"),
		QStringLiteral("Units/AbilityData.slk"),
		QStringLiteral("Units/AbilityMetaData.slk"),
		QStringLiteral("Units/AbilityBuffData.slk"),
		QStringLiteral("Units/AbilityBuffMetaData.slk"),
		QStringLiteral("Units/UpgradeData.slk"),
		QStringLiteral("Units/UpgradeMetaData.slk"),
		QStringLiteral("Units/MiscMetaData.slk"),
		QStringLiteral("Units/MiscData.txt"),
		QStringLiteral("Units/MiscGame.txt"),
		QStringLiteral("Units/UnitSkin.txt"),
		QStringLiteral("Units/UnitWeaponsFunc.txt"),
		QStringLiteral("Units/UnitWeaponsSkin.txt"),
		QStringLiteral("Units/ItemSkin.txt"),
		QStringLiteral("Units/ItemFunc.txt"),
		QStringLiteral("Units/ItemStrings.txt"),
		QStringLiteral("Doodads/DoodadSkins.txt"),
		QStringLiteral("Units/AbilitySkin.txt"),
		QStringLiteral("Units/AbilitySkinStrings.txt"),
		QStringLiteral("UI/UnitEditorData.txt")
	};

	int loaded = 0;
	for (const QString& file : files) {
		GameDataFileInfo info = loadOne(
			mapDirectory,
			result.cascOpened ? &storage : nullptr,
			mpqOpened ? &mpq : nullptr,
			file,
			tileset);
		if (info.loaded) {
			++loaded;
		}
		result.files.insert(file, info);
	}

	result.log.push_back(QStringLiteral("game data: storage=%1 files=%2/%3 path=%4")
		.arg(result.cascOpened ? QStringLiteral("CASC") : QStringLiteral("MPQ"))
		.arg(loaded)
		.arg(files.size())
		.arg(result.storagePath));
	return result;
}
