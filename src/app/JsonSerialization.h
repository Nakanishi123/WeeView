#pragma once

#include "app/AppSettings.h"
#include "model/CoreTypes.h"

#include <QJsonObject>

namespace weeview::json {

QJsonObject toJson(const AppSettings &settings);
AppSettings appSettingsFromJson(const QJsonObject &object);

QJsonObject toJson(const HistoryEntry &entry);
HistoryEntry historyEntryFromJson(const QJsonObject &object);

QString toJsonString(ViewMode viewMode);
QString toJsonString(ReadingDirection readingDirection);
QString toJsonString(BookType bookType);
QString toJsonString(SpreadGroupDirection spreadGroupDirection);

ViewMode viewModeFromJsonString(const QString &value, ViewMode fallback);
ReadingDirection readingDirectionFromJsonString(const QString &value, ReadingDirection fallback);
BookType bookTypeFromJsonString(const QString &value, BookType fallback);
SpreadGroupDirection spreadGroupDirectionFromJsonString(const QString &value, SpreadGroupDirection fallback);

} // namespace weeview::json
