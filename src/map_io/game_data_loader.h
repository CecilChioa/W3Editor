#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

using GameDataRecord = QHash<QString, QString>;

struct GameDataFileInfo {
	bool loaded = false;
	int bytes = 0;
	int rows = 0;
	int sections = 0;
	QString source;
	QString error;
	QHash<QString, GameDataRecord> records;
	QHash<QString, GameDataRecord> sectionValues;
	QStringList sampleKeys;
};

struct GameDataLoadResult {
	bool cascOpened = false;
	QString warcraftDirectory;
	QString storagePath;
	QStringList log;
	QHash<QString, GameDataFileInfo> files;
};

class GameDataLoader {
public:
	static GameDataLoadResult load(const QString& mapDirectory, const QString& tileset);
};
