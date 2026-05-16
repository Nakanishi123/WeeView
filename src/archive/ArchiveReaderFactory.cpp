#include "ArchiveReaderFactory.h"

#include "archive/SevenZipArchiveReader.h"
#include "archive/ZipArchiveReader.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>

namespace weeview {
namespace {

bool isSevenZipSuffix(const QString &suffix) { return suffix == QLatin1String("7z") || suffix == QLatin1String("cb7"); }

bool isZipSuffix(const QString &suffix) { return suffix == QLatin1String("zip") || suffix == QLatin1String("cbz"); }

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

    if ((isSevenZipSuffix(suffix) || isZipSuffix(suffix)) && hasSevenZipSignature(archivePath)) {
        return std::make_unique<SevenZipArchiveReader>(archivePath);
    }

    if (isSevenZipSuffix(suffix)) {
        auto zipReader = std::make_unique<ZipArchiveReader>(archivePath);
        if (zipReader->isOpen()) {
            return zipReader;
        }
        return std::make_unique<SevenZipArchiveReader>(archivePath);
    }

    return std::make_unique<ZipArchiveReader>(archivePath);
}

} // namespace weeview
