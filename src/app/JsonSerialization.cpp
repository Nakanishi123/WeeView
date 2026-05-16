#include "JsonSerialization.h"

#include <QDateTime>
#include <QJsonValue>

#include <algorithm>

namespace weeview::json {
namespace {

constexpr auto schemaVersionKey = "schemaVersion";
constexpr auto homeFolderKey = "homeFolder";
constexpr auto defaultReadingDirectionKey = "defaultReadingDirection";
constexpr auto defaultViewModeKey = "defaultViewMode";
constexpr auto overlayEdgeTriggerSizeKey = "overlayEdgeTriggerSize";
constexpr auto overlayHideDelayMsKey = "overlayHideDelayMs";
constexpr auto pageLoadDebounceMsKey = "pageLoadDebounceMs";
constexpr auto imageCacheMemoryLimitMiBKey = "imageCacheMemoryLimitMiB";
constexpr auto sidebarWidthKey = "sidebarWidth";
constexpr auto windowWidthKey = "windowWidth";
constexpr auto windowHeightKey = "windowHeight";
constexpr auto windowMaximizedKey = "windowMaximized";
constexpr auto sidebarFolderSortsKey = "sidebarFolderSorts";
constexpr auto sidebarSortKeyKey = "key";
constexpr auto sidebarSortOrderKey = "order";

constexpr auto bookPathKey = "bookPath";
constexpr auto bookTypeKey = "bookType";
constexpr auto displayNameKey = "displayName";
constexpr auto lastPageIndexKey = "lastPageIndex";
constexpr auto lastDisplayLastPageIndexKey = "lastDisplayLastPageIndex";
constexpr auto pageCountKey = "pageCount";
constexpr auto viewModeKey = "viewMode";
constexpr auto readingDirectionKey = "readingDirection";
constexpr auto spreadGroupDirectionKey = "spreadGroupDirection";
constexpr auto lastOpenedAtKey = "lastOpenedAt";

QString stringValue(const QJsonObject &object, const char *key, const QString &fallback) {
    const auto value = object.value(QLatin1String(key));
    return value.isString() ? value.toString() : fallback;
}

int intValue(const QJsonObject &object, const char *key, int fallback) {
    const auto value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toInt() : fallback;
}

bool boolValue(const QJsonObject &object, const char *key, bool fallback) {
    const auto value = object.value(QLatin1String(key));
    return value.isBool() ? value.toBool() : fallback;
}

QString toJsonString(SidebarSortKey sortKey) {
    switch (sortKey) {
    case SidebarSortKey::FileName:
        return QStringLiteral("fileName");
    case SidebarSortKey::CreatedAt:
        return QStringLiteral("createdAt");
    case SidebarSortKey::ModifiedAt:
        return QStringLiteral("modifiedAt");
    }
    return QStringLiteral("fileName");
}

QString toJsonString(SidebarSortOrder sortOrder) {
    switch (sortOrder) {
    case SidebarSortOrder::Ascending:
        return QStringLiteral("ascending");
    case SidebarSortOrder::Descending:
        return QStringLiteral("descending");
    }
    return QStringLiteral("ascending");
}

SidebarSortKey sidebarSortKeyFromJsonString(const QString &value, SidebarSortKey fallback) {
    if (value == QLatin1String("fileName")) {
        return SidebarSortKey::FileName;
    }
    if (value == QLatin1String("createdAt")) {
        return SidebarSortKey::CreatedAt;
    }
    if (value == QLatin1String("modifiedAt")) {
        return SidebarSortKey::ModifiedAt;
    }
    return fallback;
}

SidebarSortOrder sidebarSortOrderFromJsonString(const QString &value, SidebarSortOrder fallback) {
    if (value == QLatin1String("ascending")) {
        return SidebarSortOrder::Ascending;
    }
    if (value == QLatin1String("descending")) {
        return SidebarSortOrder::Descending;
    }
    return fallback;
}

QJsonObject toJson(const SidebarSortSettings &settings) {
    return {
        {QLatin1String(sidebarSortKeyKey), toJsonString(settings.key)},
        {QLatin1String(sidebarSortOrderKey), toJsonString(settings.order)},
    };
}

SidebarSortSettings sidebarSortSettingsFromJson(const QJsonObject &object) {
    SidebarSortSettings settings;
    settings.key =
        sidebarSortKeyFromJsonString(stringValue(object, sidebarSortKeyKey, toJsonString(settings.key)), settings.key);
    settings.order = sidebarSortOrderFromJsonString(
        stringValue(object, sidebarSortOrderKey, toJsonString(settings.order)), settings.order);
    return settings;
}

} // namespace

QJsonObject toJson(const AppSettings &settings) {
    QJsonObject sidebarFolderSorts;
    for (auto it = settings.sidebarFolderSorts.cbegin(); it != settings.sidebarFolderSorts.cend(); ++it) {
        if (!it.key().isEmpty()) {
            sidebarFolderSorts.insert(it.key(), toJson(it.value()));
        }
    }

    return {
        {QLatin1String(schemaVersionKey), settings.schemaVersion},
        {QLatin1String(homeFolderKey), settings.homeFolder},
        {QLatin1String(defaultReadingDirectionKey), toJsonString(settings.defaultReadingDirection)},
        {QLatin1String(defaultViewModeKey), toJsonString(settings.defaultViewMode)},
        {QLatin1String(overlayEdgeTriggerSizeKey), settings.overlayEdgeTriggerSize},
        {QLatin1String(overlayHideDelayMsKey), settings.overlayHideDelayMs},
        {QLatin1String(pageLoadDebounceMsKey), settings.pageLoadDebounceMs},
        {QLatin1String(imageCacheMemoryLimitMiBKey), settings.imageCacheMemoryLimitMiB},
        {QLatin1String(sidebarWidthKey), settings.sidebarWidth},
        {QLatin1String(windowWidthKey), settings.windowWidth},
        {QLatin1String(windowHeightKey), settings.windowHeight},
        {QLatin1String(windowMaximizedKey), settings.windowMaximized},
        {QLatin1String(sidebarFolderSortsKey), sidebarFolderSorts},
    };
}

AppSettings appSettingsFromJson(const QJsonObject &object) {
    AppSettings settings;
    settings.schemaVersion = intValue(object, schemaVersionKey, settings.schemaVersion);

    const auto homeFolder = stringValue(object, homeFolderKey, settings.homeFolder).trimmed();
    settings.homeFolder = homeFolder.isEmpty() ? settings.homeFolder : homeFolder;

    settings.defaultReadingDirection = readingDirectionFromJsonString(
        stringValue(object, defaultReadingDirectionKey, toJsonString(settings.defaultReadingDirection)),
        settings.defaultReadingDirection);
    settings.defaultViewMode = viewModeFromJsonString(
        stringValue(object, defaultViewModeKey, toJsonString(settings.defaultViewMode)), settings.defaultViewMode);
    settings.overlayEdgeTriggerSize = intValue(object, overlayEdgeTriggerSizeKey, settings.overlayEdgeTriggerSize);
    settings.overlayHideDelayMs = intValue(object, overlayHideDelayMsKey, settings.overlayHideDelayMs);
    settings.pageLoadDebounceMs = intValue(object, pageLoadDebounceMsKey, settings.pageLoadDebounceMs);
    settings.imageCacheMemoryLimitMiB =
        std::max(1, intValue(object, imageCacheMemoryLimitMiBKey, settings.imageCacheMemoryLimitMiB));
    settings.sidebarWidth = intValue(object, sidebarWidthKey, settings.sidebarWidth);
    settings.windowWidth = intValue(object, windowWidthKey, settings.windowWidth);
    settings.windowHeight = intValue(object, windowHeightKey, settings.windowHeight);
    settings.windowMaximized = boolValue(object, windowMaximizedKey, settings.windowMaximized);

    const auto sidebarFolderSortsValue = object.value(QLatin1String(sidebarFolderSortsKey));
    if (sidebarFolderSortsValue.isObject()) {
        const auto sidebarFolderSorts = sidebarFolderSortsValue.toObject();
        for (auto it = sidebarFolderSorts.constBegin(); it != sidebarFolderSorts.constEnd(); ++it) {
            if (it.key().isEmpty() || !it.value().isObject()) {
                continue;
            }
            settings.sidebarFolderSorts.insert(it.key(), sidebarSortSettingsFromJson(it.value().toObject()));
        }
    }
    return settings;
}

QJsonObject toJson(const HistoryEntry &entry) {
    return {
        {QLatin1String(bookPathKey), entry.bookPath},
        {QLatin1String(bookTypeKey), toJsonString(entry.bookType)},
        {QLatin1String(displayNameKey), entry.displayName},
        {QLatin1String(lastPageIndexKey), entry.lastPageIndex},
        {QLatin1String(lastDisplayLastPageIndexKey), entry.lastDisplayLastPageIndex},
        {QLatin1String(pageCountKey), entry.pageCount},
        {QLatin1String(viewModeKey), toJsonString(entry.viewMode)},
        {QLatin1String(readingDirectionKey), toJsonString(entry.readingDirection)},
        {QLatin1String(spreadGroupDirectionKey), toJsonString(entry.spreadGroupDirection)},
        {QLatin1String(lastOpenedAtKey), entry.lastOpenedAt.toUTC().toString(Qt::ISODateWithMs)},
    };
}

HistoryEntry historyEntryFromJson(const QJsonObject &object) {
    HistoryEntry entry;
    entry.bookPath = stringValue(object, bookPathKey, entry.bookPath);
    entry.bookType =
        bookTypeFromJsonString(stringValue(object, bookTypeKey, toJsonString(entry.bookType)), entry.bookType);
    entry.displayName = stringValue(object, displayNameKey, entry.displayName);
    entry.lastPageIndex = intValue(object, lastPageIndexKey, entry.lastPageIndex);
    entry.lastDisplayLastPageIndex = intValue(object, lastDisplayLastPageIndexKey, entry.lastDisplayLastPageIndex);
    entry.pageCount = intValue(object, pageCountKey, entry.pageCount);
    entry.viewMode =
        viewModeFromJsonString(stringValue(object, viewModeKey, toJsonString(entry.viewMode)), entry.viewMode);
    entry.readingDirection = readingDirectionFromJsonString(
        stringValue(object, readingDirectionKey, toJsonString(entry.readingDirection)), entry.readingDirection);
    entry.spreadGroupDirection = spreadGroupDirectionFromJsonString(
        stringValue(object, spreadGroupDirectionKey, toJsonString(entry.spreadGroupDirection)),
        entry.spreadGroupDirection);
    entry.lastOpenedAt = QDateTime::fromString(stringValue(object, lastOpenedAtKey, {}), Qt::ISODateWithMs);
    if (!object.contains(QLatin1String(lastDisplayLastPageIndexKey))) {
        entry.lastDisplayLastPageIndex = entry.lastPageIndex;
    }
    return entry;
}

QString toJsonString(ViewMode viewMode) {
    switch (viewMode) {
    case ViewMode::SinglePage:
        return QStringLiteral("singlePage");
    case ViewMode::Spread:
        return QStringLiteral("spread");
    }
    return QStringLiteral("singlePage");
}

QString toJsonString(ReadingDirection readingDirection) {
    switch (readingDirection) {
    case ReadingDirection::RightToLeft:
        return QStringLiteral("rightToLeft");
    case ReadingDirection::LeftToRight:
        return QStringLiteral("leftToRight");
    }
    return QStringLiteral("rightToLeft");
}

QString toJsonString(BookType bookType) {
    switch (bookType) {
    case BookType::Folder:
        return QStringLiteral("folder");
    case BookType::Archive:
        return QStringLiteral("archive");
    }
    return QStringLiteral("folder");
}

QString toJsonString(SpreadGroupDirection spreadGroupDirection) {
    switch (spreadGroupDirection) {
    case SpreadGroupDirection::Forward:
        return QStringLiteral("forward");
    case SpreadGroupDirection::Backward:
        return QStringLiteral("backward");
    }
    return QStringLiteral("forward");
}

ViewMode viewModeFromJsonString(const QString &value, ViewMode fallback) {
    if (value == QLatin1String("singlePage")) {
        return ViewMode::SinglePage;
    }
    if (value == QLatin1String("spread")) {
        return ViewMode::Spread;
    }
    return fallback;
}

ReadingDirection readingDirectionFromJsonString(const QString &value, ReadingDirection fallback) {
    if (value == QLatin1String("rightToLeft")) {
        return ReadingDirection::RightToLeft;
    }
    if (value == QLatin1String("leftToRight")) {
        return ReadingDirection::LeftToRight;
    }
    return fallback;
}

BookType bookTypeFromJsonString(const QString &value, BookType fallback) {
    if (value == QLatin1String("folder")) {
        return BookType::Folder;
    }
    if (value == QLatin1String("archive") || value == QLatin1String("zip")) {
        return BookType::Archive;
    }
    return fallback;
}

SpreadGroupDirection spreadGroupDirectionFromJsonString(const QString &value, SpreadGroupDirection fallback) {
    if (value == QLatin1String("forward")) {
        return SpreadGroupDirection::Forward;
    }
    if (value == QLatin1String("backward")) {
        return SpreadGroupDirection::Backward;
    }
    return fallback;
}

} // namespace weeview::json
