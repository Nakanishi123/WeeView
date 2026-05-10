#include "AppSettingsStore.h"

#include "app/JsonSerialization.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include <utility>

namespace weeview {
namespace {

QString defaultSettingsPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
           QDir::separator() + QStringLiteral("settings.json");
}

bool ensureParentDirectoryExists(const QString &filePath) {
    const auto directory = QFileInfo(filePath).absolutePath();
    return QDir().mkpath(directory);
}

} // namespace

AppSettingsStore::AppSettingsStore()
    : AppSettingsStore(defaultSettingsPath()) {}

AppSettingsStore::AppSettingsStore(QString filePath)
    : filePath_(std::move(filePath)) {}

AppSettings AppSettingsStore::load() const {
    QFile file(filePath_);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return {};
    }

    return json::appSettingsFromJson(document.object());
}

bool AppSettingsStore::save(const AppSettings &settings) const {
    if (!ensureParentDirectoryExists(filePath_)) {
        return false;
    }

    QSaveFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    const QJsonDocument document(json::toJson(settings));
    if (file.write(document.toJson(QJsonDocument::Indented)) == -1) {
        return false;
    }
    return file.commit();
}

QString AppSettingsStore::filePath() const { return filePath_; }

} // namespace weeview
