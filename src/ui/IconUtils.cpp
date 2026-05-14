#include "IconUtils.h"

#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace weeview::icons {
namespace {

QPixmap renderSvgPixmap(const QString &path, int size) {
    const auto pixmap = QIcon(path).pixmap(QSize(size, size));
    if (!pixmap.isNull()) {
        return pixmap;
    }

    QPixmap fallback(size, size);
    fallback.fill(Qt::transparent);
    return fallback;
}

QPixmap expandedAlphaPixmap(const QPixmap &source, int growth) {
    if (growth <= 0) {
        return source;
    }

    QPixmap expanded(source.size());
    expanded.fill(Qt::transparent);

    QPainter painter(&expanded);
    painter.setRenderHint(QPainter::Antialiasing);
    for (int y = -growth; y <= growth; ++y) {
        for (int x = -growth; x <= growth; ++x) {
            if ((x * x) + (y * y) <= growth * growth) {
                painter.drawPixmap(x, y, source);
            }
        }
    }
    return expanded;
}

QPixmap colorizePixmap(const QPixmap &source, const QColor &color) {
    QPixmap colorized(source.size());
    colorized.fill(Qt::transparent);

    QPainter painter(&colorized);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(colorized.rect(), color);
    return colorized;
}

} // namespace

QIcon tintedSvgIcon(const QString &path, const QColor &color, int size, int growth) {
    return QIcon(colorizePixmap(expandedAlphaPixmap(renderSvgPixmap(path, size), growth), color));
}

} // namespace weeview::icons
