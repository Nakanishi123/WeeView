#include "FolderBook.h"

#include "image/ImageDecoder.h"
#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace weeview {

FolderBook::FolderBook(QString folderPath) : folderPath_(QFileInfo(folderPath).absoluteFilePath()) {
    const QFileInfo folderInfo(folderPath_);
    displayName_ = folderInfo.fileName().isEmpty() ? folderPath_ : folderInfo.fileName();

    if (folderInfo.isDir()) {
        scanPages();
    }
}

BookType FolderBook::type() const { return BookType::Folder; }

QString FolderBook::displayName() const { return displayName_; }

QString FolderBook::sourcePath() const { return folderPath_; }

int FolderBook::pageCount() const { return pages_.size(); }

PageInfo FolderBook::pageInfo(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }
    return pages_.at(pageIndex).info;
}

QImage FolderBook::loadPage(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }

    return ImageDecoder().readFile(pages_.at(pageIndex).filePath);
}

void FolderBook::scanPages() {
    QDir directory(folderPath_);
    const auto entries = directory.entryInfoList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::NoSort);

    QStringList pagePaths;
    for (const auto &entry : entries) {
        if (filetypes::isSupportedImageFile(entry.fileName())) {
            pagePaths.append(entry.absoluteFilePath());
        }
    }

    naturalsort::sort(pagePaths);

    pages_.reserve(pagePaths.size());
    for (const auto &pagePath : pagePaths) {
        const auto imageSize = ImageDecoder().imageSize(pagePath);

        const QFileInfo pageFile(pagePath);
        pages_.append({
            pagePath,
            {
                pageFile.fileName(),
                pagePath,
                imageSize,
                imageSize.isValid() && imageSize.width() > imageSize.height(),
            },
        });
    }
}

bool FolderBook::isValidPageIndex(int pageIndex) const { return pageIndex >= 0 && pageIndex < pages_.size(); }

} // namespace weeview
