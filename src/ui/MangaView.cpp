#include "MangaView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QRectF>

#include <algorithm>
#include <utility>

namespace weeview {

MangaView::MangaView(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);

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

void MangaView::setPageCount(int pageCount) {
    const auto normalizedPageCount = std::max(0, pageCount);
    if (pageCount_ == normalizedPageCount) {
        return;
    }

    pageCount_ = normalizedPageCount;
    emit pageCountChanged(pageCount_);
    setCurrentPageIndex(currentPageIndex_);
}

void MangaView::setCurrentPageIndex(int pageIndex) {
    const auto nextPageIndex = clampedPageIndex(pageIndex);
    if (currentPageIndex_ == nextPageIndex) {
        return;
    }

    currentPageIndex_ = nextPageIndex;
    emit currentPageIndexChanged(currentPageIndex_);
}

void MangaView::setViewMode(ViewMode viewMode) {
    if (viewMode_ == viewMode) {
        return;
    }

    viewMode_ = viewMode;
    emit viewModeChanged(viewMode_);
}

void MangaView::setReadingDirection(ReadingDirection readingDirection) {
    if (readingDirection_ == readingDirection) {
        return;
    }

    readingDirection_ = readingDirection;
    emit readingDirectionChanged(readingDirection_);
}

void MangaView::setViewerState(const ViewerState &state) {
    setViewMode(state.viewMode);
    setReadingDirection(state.readingDirection);
    setCurrentPageIndex(state.currentPageIndex);
}

const QImage &MangaView::image() const { return image_; }

int MangaView::pageCount() const { return pageCount_; }

int MangaView::currentPageIndex() const { return currentPageIndex_; }

ViewMode MangaView::viewMode() const { return viewMode_; }

ReadingDirection MangaView::readingDirection() const { return readingDirection_; }

ViewerState MangaView::viewerState() const {
    return {
        currentPageIndex_,
        viewMode_,
        readingDirection_,
    };
}

void MangaView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    if (image_.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawImage(fittedImageRect(), image_);
}

void MangaView::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Left:
        goToDirectionAwareLeft();
        event->accept();
        return;
    case Qt::Key_Right:
        goToDirectionAwareRight();
        event->accept();
        return;
    case Qt::Key_Space:
    case Qt::Key_PageDown:
        goToNextPage();
        event->accept();
        return;
    case Qt::Key_Backspace:
    case Qt::Key_PageUp:
        goToPreviousPage();
        event->accept();
        return;
    case Qt::Key_Home:
        goToFirstPage();
        event->accept();
        return;
    case Qt::Key_End:
        goToLastPage();
        event->accept();
        return;
    default:
        QWidget::keyPressEvent(event);
    }
}

void MangaView::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    setFocus(Qt::MouseFocusReason);

    if (event->position().x() < width() / 2.0) {
        goToDirectionAwareLeft();
    } else {
        goToDirectionAwareRight();
    }
    event->accept();
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

int MangaView::clampedPageIndex(int pageIndex) const {
    if (!hasPages()) {
        return 0;
    }
    return std::clamp(pageIndex, 0, pageCount_ - 1);
}

bool MangaView::hasPages() const { return pageCount_ > 0; }

void MangaView::goToFirstPage() { setCurrentPageIndex(0); }

void MangaView::goToLastPage() { setCurrentPageIndex(pageCount_ - 1); }

void MangaView::goToNextPage() { setCurrentPageIndex(currentPageIndex_ + 1); }

void MangaView::goToPreviousPage() { setCurrentPageIndex(currentPageIndex_ - 1); }

void MangaView::goToDirectionAwareLeft() {
    if (readingDirection_ == ReadingDirection::RightToLeft) {
        goToNextPage();
    } else {
        goToPreviousPage();
    }
}

void MangaView::goToDirectionAwareRight() {
    if (readingDirection_ == ReadingDirection::RightToLeft) {
        goToPreviousPage();
    } else {
        goToNextPage();
    }
}

} // namespace weeview
