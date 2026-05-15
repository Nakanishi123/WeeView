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

} // namespace

QJsonObject toJson(const AppSettings &settings) {
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
    case BookType::Zip:
        return QStringLiteral("zip");
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
    if (value == QLatin1String("zip")) {
        return BookType::Zip;
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
