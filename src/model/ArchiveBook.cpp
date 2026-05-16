#include "ArchiveBook.h"

#include "archive/ArchiveReaderFactory.h"
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

ArchiveBook::ArchiveBook(QString archivePath)
    : archivePath_(QFileInfo(archivePath).absoluteFilePath()), displayName_(QFileInfo(archivePath_).fileName()),
      reader_(createArchiveReader(archivePath_)) {
    if (reader_ && reader_->isOpen()) {
        scanPages();
    }
}

ArchiveBook::~ArchiveBook() = default;

ArchiveBook::ArchiveBook(ArchiveBook &&) noexcept = default;

ArchiveBook &ArchiveBook::operator=(ArchiveBook &&) noexcept = default;

BookType ArchiveBook::type() const { return BookType::Archive; }

QString ArchiveBook::displayName() const { return displayName_; }

QString ArchiveBook::sourcePath() const { return archivePath_; }

int ArchiveBook::pageCount() const { return pages_.size(); }

PageInfo ArchiveBook::pageInfo(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }
    return pages_.at(pageIndex).info;
}

PageInfo ArchiveBook::loadPageInfo(int pageIndex) const {
    if (!isValidPageIndex(pageIndex) || !reader_) {
        return {};
    }

    auto info = pages_.at(pageIndex).info;
    info.imageSize =
        ImageDecoder().imageSize(reader_->readFile(pages_.at(pageIndex).entryPath), pages_.at(pageIndex).entryPath);
    info.isLandscape = info.imageSize.isValid() && info.imageSize.width() > info.imageSize.height();
    return info;
}

QImage ArchiveBook::loadPage(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }
    return imageFromEntry(pages_.at(pageIndex).entryPath);
}

void ArchiveBook::scanPages() {
    if (!reader_) {
        return;
    }

    QStringList pageEntryPaths;
    for (const auto &entry : reader_->entries()) {
        if (!isIgnoredArchivePath(entry.path) && filetypes::isSupportedImageFile(entry.path)) {
            pageEntryPaths.append(entry.path);
        }
    }

    naturalsort::sort(pageEntryPaths);

    pages_.reserve(pageEntryPaths.size());
    for (const auto &entryPath : pageEntryPaths) {
        pages_.append({
            entryPath,
            {
                QFileInfo(entryPath).fileName(),
                entryPath,
                {},
                false,
            },
        });
    }
}

bool ArchiveBook::isValidPageIndex(int pageIndex) const { return pageIndex >= 0 && pageIndex < pages_.size(); }

QImage ArchiveBook::imageFromEntry(const QString &entryPath) const {
    return reader_ ? ImageDecoder().readData(reader_->readFile(entryPath), entryPath) : QImage();
}

} // namespace weeview
