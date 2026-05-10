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
};

} // namespace weeview
