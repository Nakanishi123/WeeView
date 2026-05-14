#pragma once

#include "model/CoreTypes.h"

#include <QDir>
#include <QString>

namespace weeview {

struct AppSettings {
    int schemaVersion = 1;
    QString homeFolder = QDir::homePath();
    ReadingDirection defaultReadingDirection = ReadingDirection::RightToLeft;
    ViewMode defaultViewMode = ViewMode::SinglePage;
    int overlayEdgeTriggerSize = 24;
    int overlayHideDelayMs = 800;
    int pageLoadDebounceMs = 120;
    int sidebarWidth = 320;
    int windowWidth = 960;
    int windowHeight = 720;
    bool windowMaximized = false;
};

} // namespace weeview
