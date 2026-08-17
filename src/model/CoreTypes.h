#pragma once

#include <QDateTime>
#include <QSize>
#include <QString>

namespace weeview {

enum class ViewMode {
    SinglePage,
    Spread,
};

enum class ReadingDirection {
    RightToLeft,
    LeftToRight,
};

enum class BookType {
    Folder,
    Archive,
};

enum class SidebarSortKey {
    FileName,
    CreatedAt,
    ModifiedAt,
};

enum class SidebarSortOrder {
    Ascending,
    Descending,
};

enum class SpreadGroupDirection {
    Forward,
    Backward,
};

struct PageInfo {
    QString imageName;
    QString displayPath;
    QSize imageSize;
    bool isLandscape = false;
};

struct ViewerState {
    int currentPageIndex = 0;
    int currentDisplayLastPageIndex = 0;
    ViewMode viewMode = ViewMode::SinglePage;
    ReadingDirection readingDirection = ReadingDirection::RightToLeft;
    SpreadGroupDirection spreadGroupDirection = SpreadGroupDirection::Forward;
};

struct SidebarSortSettings {
    SidebarSortKey key = SidebarSortKey::FileName;
    SidebarSortOrder order = SidebarSortOrder::Ascending;
};

struct HistoryEntry {
    QString bookPath;
    BookType bookType = BookType::Folder;
    QString displayName;
    int lastPageIndex = 0;
    int lastDisplayLastPageIndex = 0;
    int pageCount = 0;
    ViewMode viewMode = ViewMode::SinglePage;
    ReadingDirection readingDirection = ReadingDirection::RightToLeft;
    SpreadGroupDirection spreadGroupDirection = SpreadGroupDirection::Forward;
    QDateTime lastOpenedAt;
};

} // namespace weeview
