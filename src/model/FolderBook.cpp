#include "FolderBook.h"

#include "image/ImageDecoder.h"
#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>
#include <utility>

namespace weeview {

FolderBook::FolderBook(QString folderPath, SidebarSortSettings sortSettings)
    : folderPath_(QFileInfo(folderPath).absoluteFilePath()), sortSettings_(sortSettings) {
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

PageInfo FolderBook::loadPageInfo(int pageIndex) const {
    if (!isValidPageIndex(pageIndex)) {
        return {};
    }

    auto info = pages_.at(pageIndex).info;
    info.imageSize = ImageDecoder().imageSize(pages_.at(pageIndex).filePath);
    info.isLandscape = info.imageSize.isValid() && info.imageSize.width() > info.imageSize.height();
    return info;
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

    QVector<Page> pages;
    for (const auto &entry : entries) {
        if (filetypes::isSupportedImageFile(entry.fileName())) {
            pages.append({
                entry.absoluteFilePath(),
                {
                    entry.fileName(),
                    entry.absoluteFilePath(),
                    {},
                    false,
                },
            });
        }
    }

    std::sort(pages.begin(), pages.end(), [this](const Page &left, const Page &right) {
        const QFileInfo leftFile(left.filePath);
        const QFileInfo rightFile(right.filePath);

        auto result = 0;
        switch (sortSettings_.key) {
        case SidebarSortKey::FileName:
            result = naturalsort::lessThan(left.info.imageName, right.info.imageName)
                         ? -1
                         : (naturalsort::lessThan(right.info.imageName, left.info.imageName) ? 1 : 0);
            break;
        case SidebarSortKey::CreatedAt: {
            const auto leftCreatedAt =
                leftFile.birthTime().isValid() ? leftFile.birthTime() : leftFile.metadataChangeTime();
            const auto rightCreatedAt =
                rightFile.birthTime().isValid() ? rightFile.birthTime() : rightFile.metadataChangeTime();
            result = leftCreatedAt < rightCreatedAt ? -1 : (rightCreatedAt < leftCreatedAt ? 1 : 0);
            break;
        }
        case SidebarSortKey::ModifiedAt:
            result = leftFile.lastModified() < rightFile.lastModified()
                         ? -1
                         : (rightFile.lastModified() < leftFile.lastModified() ? 1 : 0);
            break;
        }

        if (result == 0) {
            result = naturalsort::lessThan(left.info.imageName, right.info.imageName)
                         ? -1
                         : (naturalsort::lessThan(right.info.imageName, left.info.imageName) ? 1 : 0);
        }

        return sortSettings_.order == SidebarSortOrder::Ascending ? result < 0 : result > 0;
    });

    pages_ = std::move(pages);
}

bool FolderBook::isValidPageIndex(int pageIndex) const { return pageIndex >= 0 && pageIndex < pages_.size(); }

} // namespace weeview
