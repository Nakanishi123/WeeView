#include "ZipBook.h"

#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QBuffer>
#include <QFileInfo>
#include <QIODevice>
#include <QImageReader>
#include <QStringList>

namespace weeview {
namespace {

bool isIgnoredArchivePath(const QString &entryPath) {
    return entryPath.startsWith(QStringLiteral("__MACOSX/")) || entryPath.endsWith(QStringLiteral("/.DS_Store")) ||
           entryPath == QStringLiteral(".DS_Store");
}

QSize imageSizeFromData(const QByteArray &data) {
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return {};
    }

    QImageReader reader(&buffer);
    return reader.size();
}

QImage imageFromData(const QByteArray &data) {
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return {};
    }

    QImageReader reader(&buffer);
    return reader.read();
}

} // namespace

ZipBook::ZipBook(QString archivePath)
    : archivePath_(QFileInfo(archivePath).absoluteFilePath()), displayName_(QFileInfo(archivePath_).fileName()),
      reader_(archivePath_) {
    if (reader_.isOpen()) {
        scanPages();
    }
}

BookType ZipBook::type() const { return BookType::Zip; }

QString ZipBook::displayName() const { return displayName_; }

QString ZipBook::sourcePath() const { return archivePath_; }

int ZipBook::pageCount() const { return pages_.size(); }

PageInfo ZipBook::pageInfo(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }
    return pages_.at(pageIndex).info;
}

QImage ZipBook::loadPage(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }
    return imageFromEntry(pages_.at(pageIndex).entryPath);
}

void ZipBook::scanPages() {
    QStringList pageEntryPaths;
    for (const auto &entry : reader_.entries()) {
        if (!isIgnoredArchivePath(entry.path) && filetypes::isSupportedImageFile(entry.path)) {
            pageEntryPaths.append(entry.path);
        }
    }

    naturalsort::sort(pageEntryPaths);

    pages_.reserve(pageEntryPaths.size());
    for (const auto &entryPath : pageEntryPaths) {
        const auto imageSize = imageSizeFromData(reader_.readFile(entryPath));
        pages_.append({
            entryPath,
            {
                QFileInfo(entryPath).fileName(),
                entryPath,
                imageSize,
                imageSize.isValid() && imageSize.width() > imageSize.height(),
            },
        });
    }
}

bool ZipBook::isValidPageIndex(int pageIndex) const { return pageIndex >= 0 && pageIndex < pages_.size(); }

QImage ZipBook::imageFromEntry(const QString &entryPath) const { return imageFromData(reader_.readFile(entryPath)); }

} // namespace weeview
