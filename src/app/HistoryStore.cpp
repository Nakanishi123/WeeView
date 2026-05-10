#include "HistoryStore.h"

#include "app/JsonSerialization.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <utility>

namespace weeview {
namespace {

constexpr auto schemaVersionKey = "schemaVersion";
constexpr auto entriesKey = "entries";

QString defaultHistoryPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QDir::separator() + QStringLiteral("history.json");
}

bool ensureParentDirectoryExists(const QString &filePath) {
    const auto directory = QFileInfo(filePath).absolutePath();
    return QDir().mkpath(directory);
}

} // namespace

HistoryStore::HistoryStore() : HistoryStore(defaultHistoryPath()) {}

HistoryStore::HistoryStore(QString filePath) : filePath_(std::move(filePath)) {}

QVector<HistoryEntry> HistoryStore::load() const {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    const auto entriesValue =
        document.object().value(QLatin1String(entriesKey));
    if (!entriesValue.isArray()) {
        return {};
    }

    QVector<HistoryEntry> entries;
    for (const auto &entryValue : entriesValue.toArray()) {
        if (entryValue.isObject()) {
            entries.append(json::historyEntryFromJson(entryValue.toObject()));
        }
    }
    return entries;
}

bool HistoryStore::save(const QVector<HistoryEntry> &entries) const {
    if (!ensureParentDirectoryExists(filePath_)) {
        return false;
    }

    QJsonArray entryArray;
    for (const auto &entry : entries) {
        entryArray.append(json::toJson(entry));
    }

    const QJsonObject root = {
        {QLatin1String(schemaVersionKey), 1},
        {QLatin1String(entriesKey), entryArray},
    };

    QSaveFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QJsonDocument document(root);
    if (file.write(document.toJson(QJsonDocument::Indented)) == -1) {
        return false;
    }
    return file.commit();
}

QString HistoryStore::filePath() const { return filePath_; }

} // namespace weeview
