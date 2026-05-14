#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

namespace weeview::icons {

[[nodiscard]] QIcon tintedSvgIcon(const QString &path, const QColor &color, int size, int growth = 1);

} // namespace weeview::icons
