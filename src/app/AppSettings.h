#pragma once

#include "model/CoreTypes.h"

#include <QDir>
#include <QMap>
#include <QString>

namespace weeview {

enum class SidebarSortKey {
    FileName,
    CreatedAt,
    ModifiedAt,
};

enum class SidebarSortOrder {
    Ascending,
    Descending,
};

struct SidebarSortSettings {
    SidebarSortKey key = SidebarSortKey::FileName;
    SidebarSortOrder order = SidebarSortOrder::Ascending;
};

struct AppSettings {
    int schemaVersion = 1;
    QString homeFolder = QDir::homePath();
    ReadingDirection defaultReadingDirection = ReadingDirection::RightToLeft;
    ViewMode defaultViewMode = ViewMode::SinglePage;
    int overlayEdgeTriggerSize = 24;
    int overlayHideDelayMs = 800;
    int pageLoadDebounceMs = 120;
    int imageCacheMemoryLimitMiB = 256;
    int sidebarWidth = 320;
    int windowWidth = 960;
    int windowHeight = 720;
    bool windowMaximized = false;
    QMap<QString, SidebarSortSettings> sidebarFolderSorts;
};

} // namespace weeview
