#include "ZipBook.h"

#include "image/ImageDecoder.h"
#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QFileInfo>
#include <QStringList>

namespace weeview {
namespace {

bool isIgnoredArchivePath(const QString &entryPath) {
    return entryPath.startsWith(QStringLiteral("__MACOSX/")) || entryPath.endsWith(QStringLiteral("/.DS_Store")) ||
           entryPath == QStringLiteral(".DS_Store");
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
        const auto imageSize = ImageDecoder().imageSize(reader_.readFile(entryPath), entryPath);
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

QImage ZipBook::imageFromEntry(const QString &entryPath) const {
    return ImageDecoder().readData(reader_.readFile(entryPath), entryPath);
}

} // namespace weeview
