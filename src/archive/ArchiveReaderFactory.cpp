#include "ArchiveReaderFactory.h"

#include "archive/SevenZipArchiveReader.h"
#include "archive/ZipArchiveReader.h"

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
    return signature.size() == 6 && static_cast<unsigned char>(signature.at(0)) == 0x37 &&
           static_cast<unsigned char>(signature.at(1)) == 0x7A && static_cast<unsigned char>(signature.at(2)) == 0xBC &&
           static_cast<unsigned char>(signature.at(3)) == 0xAF && static_cast<unsigned char>(signature.at(4)) == 0x27 &&
           static_cast<unsigned char>(signature.at(5)) == 0x1C;
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
