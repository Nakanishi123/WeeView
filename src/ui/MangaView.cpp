#include "MangaView.h"

#include <QPainter>
#include <QPalette>
#include <QRectF>

#include <algorithm>
#include <utility>

namespace weeview {

MangaView::MangaView(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);

    auto viewPalette = palette();
    viewPalette.setColor(QPalette::Window, Qt::black);
    setPalette(viewPalette);
}

void MangaView::setImage(QImage image) {
    image_ = std::move(image);
    update();
}

void MangaView::clearImage() {
    image_ = {};
    update();
}

const QImage &MangaView::image() const { return image_; }

void MangaView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    if (image_.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(fittedImageRect(), image_);
}

QRectF MangaView::fittedImageRect() const {
    const auto availableSize = QSizeF(size());
    const auto imageSize = QSizeF(image_.size());

    if (availableSize.isEmpty() || imageSize.isEmpty()) {
        return {};
    }

    const auto scale = std::min(availableSize.width() / imageSize.width(), availableSize.height() / imageSize.height());
    const auto fittedSize = imageSize * scale;
    const auto topLeft = QPointF((availableSize.width() - fittedSize.width()) / 2.0,
                                 (availableSize.height() - fittedSize.height()) / 2.0);

    return QRectF(topLeft, fittedSize);
}

} // namespace weeview
