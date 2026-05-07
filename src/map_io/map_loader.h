#pragma once

#include "game_data_loader.h"
#include "terrain_model.h"
#include "w3e_loader.h"

#include <QHash>
#include <QRectF>
#include <QColor>
#include <QPointF>
#include <QVector>
#include <QString>
#include <QStringList>

struct MapHierarchy {
	QString mapDirectory;
	bool archiveBacked = false;
};

struct MapArchiveInfo {
	bool present = false;
	bool validHeader = false;
	bool validFooter = false;
	qint32 placeholder = 0;
	QString name;
	quint32 flags = 0;
	qint32 playersCount = 0;
	QVector<quint8> authentication;
	QString error;
};

struct FileLoadStatus {
	bool present = false;
	bool parsed = false;
	int bytes = 0;
	QString summary;
	QString error;
};

struct W3iInfo {
	bool valid = false;
	quint32 version = 0;
	quint32 mapVersion = 0;
	quint32 editorVersion = 0;
	quint32 gameVersionMajor = 0;
	quint32 gameVersionMinor = 0;
	quint32 gameVersionPatch = 0;
	quint32 gameVersionBuild = 0;
	QString name;
	QString author;
	QString description;
	QString suggestedPlayers;
	quint32 playableWidth = 0;
	quint32 playableHeight = 0;
	quint32 flags = 0;
	quint32 players = 0;
	quint32 forces = 0;
	quint32 upgrades = 0;
	quint32 tech = 0;
	quint32 randomUnitTables = 0;
	quint32 randomItemTables = 0;
	QString error;
};

struct MapPlayerInfo {
	qint32 id = 0;
	qint32 type = 0;
	qint32 race = 0;
	qint32 fixedStartPosition = 0;
	QString name;
	QPointF startPosition;
	quint32 allyLowPriorityFlags = 0;
	quint32 allyHighPriorityFlags = 0;
};

struct MapForceInfo {
	quint32 flags = 0;
	quint32 playerMaskFlags = 0;
	QString name;
};

struct MapUpgradeAvailabilityChange {
	quint32 playerFlags = 0;
	QString upgradeId;
	qint32 levelChanged = 0;
	qint32 availability = 0;
};

struct MapTechAvailabilityChange {
	quint32 playerFlags = 0;
	QString techId;
};

struct RandomTableItem {
	qint32 chancePercent = 0;
	QString id;
	QVector<QString> ids;
};

struct RandomTableSet {
	QVector<RandomTableItem> items;
};

struct RandomUnitTableInfo {
	qint32 index = 0;
	QString name;
	QVector<qint32> positions;
	QVector<RandomTableItem> units;
};

struct RandomItemTableInfo {
	qint32 index = 0;
	QString name;
	QVector<RandomTableSet> sets;
};

struct MapMetadata {
	QVector<float> cameraBounds;
	QVector<qint32> cameraBoundsPadding;
	qint32 loadingScreenPresetIndex = 0;
	QString loadingScreenCustomPath;
	QString loadingScreenText;
	QString loadingScreenTitle;
	QString loadingScreenSubtitle;
	qint32 gameDataSetIndex = 0;
	QString prologueScreenPath;
	QString prologueScreenText;
	QString prologueScreenTitle;
	QString prologueScreenSubtitle;
	qint32 terrainFogStyle = 0;
	float terrainFogStartZ = 0.0f;
	float terrainFogEndZ = 0.0f;
	float terrainFogDensity = 0.0f;
	QColor terrainFogColor;
	QString globalWeatherId;
	QString customSoundEnvironment;
	QString customLightEnvironmentTilesetId;
	QColor waterColor;
	QVector<MapPlayerInfo> players;
	QVector<MapForceInfo> forces;
	QVector<MapUpgradeAvailabilityChange> upgradeAvailabilityChanges;
	QVector<MapTechAvailabilityChange> techAvailabilityChanges;
	QVector<RandomUnitTableInfo> randomUnitTables;
	QVector<RandomItemTableInfo> randomItemTables;
};

struct WpmInfo {
	bool valid = false;
	quint32 version = 0;
	quint32 width = 0;
	quint32 height = 0;
	quint32 cells = 0;
	quint32 canWalk = 0;
	quint32 canFly = 0;
	quint32 canBuild = 0;
	quint32 ground = 0;
	quint32 unwalkable = 0;
	quint32 unflyable = 0;
	quint32 unbuildable = 0;
	quint32 water = 0;
	QVector<quint8> flags;
	QString error;
};

struct ObjectModificationSummary {
	QString fileName;
	bool present = false;
	int bytes = 0;
	bool parsed = false;
	quint32 version = 0;
	quint32 originalObjects = 0;
	quint32 originalMods = 0;
	quint32 customObjects = 0;
	quint32 customMods = 0;
};

struct ObjectModificationValue {
	QString fieldId;
	quint32 type = 0;
	quint32 level = 0;
	quint32 dataPointer = 0;
	quint32 variation = 0;
	quint32 abilityDataColumn = 0;
	QString value;
	QString parentObjectId;
};

struct ObjectModificationRecord {
	QString fileName;
	bool customTable = false;
	QString originalId;
	QString newId;
	QVector<ObjectModificationValue> modifications;
};

struct ObjectDatabaseRecord {
	QString category;
	QString id;
	QString baseId;
	bool custom = false;
	GameDataRecord fields;
};

struct ObjectDatabase {
	QHash<QString, ObjectDatabaseRecord> units;
	QHash<QString, ObjectDatabaseRecord> items;
	QHash<QString, ObjectDatabaseRecord> destructables;
	QHash<QString, ObjectDatabaseRecord> doodads;
	QHash<QString, ObjectDatabaseRecord> abilities;
	QHash<QString, ObjectDatabaseRecord> buffs;
	QHash<QString, ObjectDatabaseRecord> upgrades;
};

struct TerrainTileTextureInfo {
	QString tileId;
	QString displayName;
	QString dir;
	QString file;
	QString fullPath;
};

struct TerrainVisualDatabase {
	QHash<QString, TerrainTileTextureInfo> groundTiles;
	QHash<QString, TerrainTileTextureInfo> cliffTiles;
	QHash<QString, GameDataRecord> waterProfiles;
};

struct PlacedDoodad {
	QString id;
	quint32 variation = 0;
	QPointF position;
	float z = 0.0f;
	float angle = 0.0f;
	QPointF scale = QPointF(1.0, 1.0);
	float scaleZ = 1.0f;
	quint8 visibility = 0;
	quint8 lifePercent = 0;
	qint32 droppedItemTableIndex = -1;
	QVector<RandomTableSet> droppedItemSets;
	quint32 creationNumber = 0;
};

struct PlacedUnit {
	QString id;
	quint32 variation = 0;
	QPointF position;
	float z = 0.0f;
	float angle = 0.0f;
	QPointF scale = QPointF(1.0, 1.0);
	float scaleZ = 1.0f;
	quint8 visibility = 0;
	quint32 player = 0;
	quint8 unknown1 = 0;
	quint8 unknown2 = 0;
	qint32 hitpoints = -1;
	qint32 mana = -1;
	qint32 droppedItemTableIndex = -1;
	QVector<RandomTableSet> droppedItemSets;
	qint32 gold = 0;
	float targetAcquisitionRange = 0.0f;
	qint32 heroLevel = 0;
	qint32 heroStrength = 0;
	qint32 heroAgility = 0;
	qint32 heroIntelligence = 0;
	QVector<RandomTableItem> inventoryItems;
	QVector<RandomTableItem> abilityModifications;
	qint32 randomType = 0;
	qint32 randomAnyLevel = -1;
	quint8 randomAnyItemClass = 0;
	qint32 randomTableIndex = -1;
	qint32 randomPositionIndex = -1;
	QVector<RandomTableItem> randomUnits;
	qint32 waygateCustomTeamColor = -1;
	qint32 waygateDestinationRegionIndex = -1;
	quint32 creationNumber = 0;
};

struct TerrainDoodad {
	QString id;
	qint32 z = 0;
	qint32 x = 0;
	qint32 y = 0;
};

struct RegionInfo {
	QRectF bounds;
	QString name;
	qint32 index = 0;
	QString weatherEffectId;
	QString ambientSoundVariable;
	QColor color;
};

struct CameraInfo {
	QPointF target;
	float offsetZ = 0.0f;
	float rotation = 0.0f;
	float angleOfAttack = 0.0f;
	float distance = 0.0f;
	float roll = 0.0f;
	float fieldOfView = 0.0f;
	float farClipping = 0.0f;
	float unknown = 0.0f;
	QString name;
};

struct SoundInfo {
	QString variable;
	QString filePath;
	QString eaxEffect;
	quint32 flags = 0;
	qint32 fadeInRate = 0;
	qint32 fadeOutRate = 0;
	qint32 volume = 0;
	float pitch = 0.0f;
	float unknown1 = 0.0f;
	qint32 unknown2 = 0;
	qint32 channel = 0;
	float distanceMin = 0.0f;
	float distanceMax = 0.0f;
	float distanceCutoff = 0.0f;
	float unknown3 = 0.0f;
	float unknown4 = 0.0f;
	qint32 unknown5 = 0;
	float unknown6 = 0.0f;
	float unknown7 = 0.0f;
	float unknown8 = 0.0f;
};

struct ImportInfo {
	bool customPath = false;
	QString path;
};

struct MinimapIconInfo {
	qint32 type = 0;
	qint32 x = 0;
	qint32 y = 0;
	QColor color;
};

struct ShadowMapInfo {
	bool valid = false;
	QVector<bool> shadows;
	QString error;
};

struct MapLoadResult {
	bool loaded = false;
	QString inputPath;
	QString error;
	QStringList log;
	MapHierarchy hierarchy;
	MapArchiveInfo archive;
	W3iInfo info;
	MapMetadata metadata;
	W3eHeaderInfo terrain;
	TerrainModel terrainModel;
	WpmInfo pathing;
	GameDataLoadResult gameData;
	QHash<QString, FileLoadStatus> files;
	QVector<ObjectModificationSummary> objectModifications;
	QVector<ObjectModificationRecord> objectModificationRecords;
	ObjectDatabase objectDatabase;
	TerrainVisualDatabase terrainVisuals;
	QVector<PlacedDoodad> placedDoodads;
	QVector<PlacedUnit> placedUnits;
	QVector<TerrainDoodad> terrainDoodads;
	QVector<RegionInfo> regions;
	QVector<CameraInfo> cameras;
	QVector<SoundInfo> sounds;
	QVector<ImportInfo> imports;
	QVector<MinimapIconInfo> minimapIcons;
	ShadowMapInfo shadowMap;
};

class MapLoader {
public:
	static MapLoadResult load(const QString& path);
};
