#include "ArchiveReaderFactory.h"

#include "archive/SevenZipArchiveReader.h"
#include "archive/ZipArchiveReader.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>

namespace weeview {
namespace {

bool hasSevenZipSignature(const QString &archivePath) {
    QFile file(archivePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const auto signature = file.read(6);
    static const auto expectedSignature = QByteArray::fromHex("377abcaf271c");
    return signature == expectedSignature;
}

} // namespace

std::unique_ptr<ArchiveReader> createArchiveReader(const QString &archivePath) {
    const auto suffix = QFileInfo(archivePath).suffix().toLower();
    if (hasSevenZipSignature(archivePath) || suffix == QLatin1String("7z") || suffix == QLatin1String("cb7")) {
        return std::make_unique<SevenZipArchiveReader>(archivePath);
    }

    return std::make_unique<ZipArchiveReader>(archivePath);
}

} // namespace weeview
