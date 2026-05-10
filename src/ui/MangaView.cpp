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
    if (pageImages_.size() <= currentPageIndex_) {
        pageImages_.resize(currentPageIndex_ + 1);
    }
    pageImages_[currentPageIndex_] = image_;
    setPageCount(std::max(pageCount_, currentPageIndex_ + 1));
    update();
}

void MangaView::setPageImage(int pageIndex, QImage image) {
    if (pageIndex < 0) {
        return;
    }

    if (pageImages_.size() <= pageIndex) {
        pageImages_.resize(pageIndex + 1);
    }

    pageImages_[pageIndex] = std::move(image);
    image_ = hasImageForPage(currentPageIndex_) ? pageImages_.at(currentPageIndex_) : QImage();
    setPageCount(std::max(pageCount_, pageIndex + 1));
    update();
}

void MangaView::setPageImages(QVector<QImage> images) {
    pageImages_ = std::move(images);
    setPageCount(pageImages_.size());
    image_ = hasImageForPage(currentPageIndex_) ? pageImages_.at(currentPageIndex_) : QImage();
    update();
}

void MangaView::clearImage() {
    image_ = {};
    if (hasImageForPage(currentPageIndex_)) {
        pageImages_[currentPageIndex_] = {};
    }
    update();
}

void MangaView::clearPageImages() {
    image_ = {};
    pageImages_.clear();
    setPageCount(0);
    update();
}

void MangaView::retainPageImages(const QSet<int> &pageIndices) {
    for (int index = 0; index < pageImages_.size(); ++index) {
        if (!pageIndices.contains(index)) {
            pageImages_[index] = {};
        }
    }

    image_ = hasImageForPage(currentPageIndex_) ? pageImages_.at(currentPageIndex_) : QImage();
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
    image_ = hasImageForPage(currentPageIndex_) ? pageImages_.at(currentPageIndex_) : QImage();
    emit currentPageIndexChanged(currentPageIndex_);
    update();
}

void MangaView::setViewMode(ViewMode viewMode) {
    if (viewMode_ == viewMode) {
        return;
    }

    viewMode_ = viewMode;
    emit viewModeChanged(viewMode_);
    update();
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

const QVector<QImage> &MangaView::pageImages() const { return pageImages_; }

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

    const auto pageIndices = displayPageIndices();
    if (pageIndices.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (pageIndices.size() == 1) {
        const auto &pageImage = pageImages_.at(pageIndices.first());
        painter.drawImage(fittedImageRect(rect(), pageImage), pageImage);
        return;
    }

    const auto halfWidth = width() / 2.0;
    const QRectF leftRect(0, 0, halfWidth, height());
    const QRectF rightRect(halfWidth, 0, width() - halfWidth, height());
    const auto firstPageRect = readingDirection_ == ReadingDirection::RightToLeft ? rightRect : leftRect;
    const auto secondPageRect = readingDirection_ == ReadingDirection::RightToLeft ? leftRect : rightRect;
    const auto &firstPageImage = pageImages_.at(pageIndices.at(0));
    const auto &secondPageImage = pageImages_.at(pageIndices.at(1));

    painter.drawImage(fittedImageRect(firstPageRect, firstPageImage), firstPageImage);
    painter.drawImage(fittedImageRect(secondPageRect, secondPageImage), secondPageImage);
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

QVector<int> MangaView::displayPageIndices() const {
    if (!hasPages() || !hasImageForPage(currentPageIndex_)) {
        return {};
    }

    if (viewMode_ == ViewMode::SinglePage || isLandscapePage(currentPageIndex_)) {
        return {currentPageIndex_};
    }

    const auto nextPageIndex = currentPageIndex_ + 1;
    if (nextPageIndex >= pageCount_ || !hasImageForPage(nextPageIndex) || isLandscapePage(nextPageIndex)) {
        return {currentPageIndex_};
    }

    return {currentPageIndex_, nextPageIndex};
}

QRectF MangaView::fittedImageRect(const QRectF &availableRect, const QImage &image) const {
    const auto availableSize = availableRect.size();
    const auto imageSize = QSizeF(image.size());

    if (availableSize.isEmpty() || imageSize.isEmpty()) {
        return {};
    }

    const auto scale = std::min(availableSize.width() / imageSize.width(), availableSize.height() / imageSize.height());
    const auto fittedSize = imageSize * scale;
    const auto topLeft = QPointF(availableRect.left() + (availableSize.width() - fittedSize.width()) / 2.0,
                                 availableRect.top() + (availableSize.height() - fittedSize.height()) / 2.0);

    return QRectF(topLeft, fittedSize);
}

int MangaView::clampedPageIndex(int pageIndex) const {
    if (!hasPages()) {
        return 0;
    }
    return std::clamp(pageIndex, 0, pageCount_ - 1);
}

bool MangaView::hasPages() const { return pageCount_ > 0; }

bool MangaView::hasImageForPage(int pageIndex) const {
    return pageIndex >= 0 && pageIndex < pageImages_.size() && !pageImages_.at(pageIndex).isNull();
}

bool MangaView::isLandscapePage(int pageIndex) const {
    if (!hasImageForPage(pageIndex)) {
        return false;
    }

    const auto imageSize = pageImages_.at(pageIndex).size();
    return imageSize.width() > imageSize.height();
}

void MangaView::goToFirstPage() { setCurrentPageIndex(0); }

void MangaView::goToLastPage() { setCurrentPageIndex(pageCount_ - 1); }

void MangaView::goToNextPage() {
    const auto step = viewMode_ == ViewMode::Spread ? static_cast<int>(displayPageIndices().size()) : 1;
    setCurrentPageIndex(currentPageIndex_ + std::max(1, step));
}

void MangaView::goToPreviousPage() {
    const auto step = viewMode_ == ViewMode::Spread ? static_cast<int>(displayPageIndices().size()) : 1;
    setCurrentPageIndex(currentPageIndex_ - std::max(1, step));
}

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
