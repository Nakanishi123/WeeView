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
    Zip,
};

struct PageInfo {
    QString imageName;
    QString displayPath;
    QSize imageSize;
    bool isLandscape = false;
};

struct ViewerState {
    int currentPageIndex = 0;
    ViewMode viewMode = ViewMode::SinglePage;
    ReadingDirection readingDirection = ReadingDirection::RightToLeft;
};

struct HistoryEntry {
    QString bookPath;
    BookType bookType = BookType::Folder;
    QString displayName;
    int lastPageIndex = 0;
    int pageCount = 0;
    ViewMode viewMode = ViewMode::SinglePage;
    ReadingDirection readingDirection = ReadingDirection::RightToLeft;
    QDateTime lastOpenedAt;
};

} // namespace weeview
