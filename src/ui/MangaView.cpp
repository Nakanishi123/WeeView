#include "MangaView.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QRectF>
#include <QWheelEvent>

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
    refreshCurrentDisplayGroup();
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
    refreshCurrentDisplayGroup();
    update();
}

void MangaView::setPageImages(QVector<QImage> images) {
    pageImages_ = std::move(images);
    setPageCount(pageImages_.size());
    image_ = hasImageForPage(currentPageIndex_) ? pageImages_.at(currentPageIndex_) : QImage();
    refreshCurrentDisplayGroup();
    update();
}

void MangaView::setPageLandscapeFlags(QVector<bool> landscapePages) {
    landscapePages_ = std::move(landscapePages);
    if (landscapePages_.size() < pageCount_) {
        landscapePages_.resize(pageCount_);
    }
    refreshCurrentDisplayGroup();
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
    landscapePages_.clear();
    currentDisplayPageIndices_.clear();
    lastPaintablePageIndices_.clear();
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
    refreshCurrentDisplayGroup();
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
    setCurrentPageIndexFromGroup(forwardSpreadGroup(pageIndex), pageIndex, SpreadGroupDirection::Forward);
}

void MangaView::setCurrentPageIndexFromGroup(QVector<int> pageIndices, int fallbackPageIndex,
                                             SpreadGroupDirection direction, int displayLastFallbackPageIndex) {
    const auto nextPageIndex = pageIndices.isEmpty() ? clampedPageIndex(fallbackPageIndex) : pageIndices.first();
    const auto pageIndexChanged = currentPageIndex_ != nextPageIndex;
    const auto displayGroupChanged = currentDisplayPageIndices_ != pageIndices;
    const auto fallbackDisplayLastPageIndex =
        displayLastFallbackPageIndex >= 0 ? displayLastFallbackPageIndex : fallbackPageIndex;

    spreadGroupDirection_ = direction;
    currentDisplayLastPageIndex_ =
        pageIndices.isEmpty() ? clampedPageIndex(fallbackDisplayLastPageIndex) : pageIndices.last();
    currentDisplayPageIndices_ = std::move(pageIndices);
    if (hasImagesForPages(currentDisplayPageIndices_)) {
        lastPaintablePageIndices_ = currentDisplayPageIndices_;
    }
    currentPageIndex_ = nextPageIndex;
    image_ = hasImageForPage(currentPageIndex_) ? pageImages_.at(currentPageIndex_) : QImage();

    if (pageIndexChanged) {
        emit currentPageIndexChanged(currentPageIndex_);
    }

    if (pageIndexChanged || displayGroupChanged) {
        update();
    }
}

void MangaView::refreshCurrentDisplayGroup() {
    if (viewMode_ == ViewMode::SinglePage) {
        const auto pageIndices = hasPage(currentPageIndex_) ? QVector<int>{currentPageIndex_} : QVector<int>{};
        setCurrentPageIndexFromGroup(pageIndices, currentPageIndex_, spreadGroupDirection_);
        return;
    }

    const auto pageIndices = spreadGroupDirection_ == SpreadGroupDirection::Forward
                                 ? forwardSpreadGroup(currentPageIndex_)
                                 : backwardSpreadGroup(currentDisplayLastPageIndex_);
    setCurrentPageIndexFromGroup(pageIndices, currentPageIndex_, spreadGroupDirection_, currentDisplayLastPageIndex_);
}

void MangaView::setViewMode(ViewMode viewMode) {
    if (viewMode_ == viewMode) {
        return;
    }

    viewMode_ = viewMode;
    refreshCurrentDisplayGroup();
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
    spreadGroupDirection_ = state.spreadGroupDirection;
    currentDisplayLastPageIndex_ = state.currentDisplayLastPageIndex;
    refreshCurrentDisplayGroup();
    setCurrentPageIndexFromGroup(state.spreadGroupDirection == SpreadGroupDirection::Forward
                                     ? forwardSpreadGroup(state.currentPageIndex)
                                     : backwardSpreadGroup(state.currentDisplayLastPageIndex),
                                 state.currentPageIndex, state.spreadGroupDirection, state.currentDisplayLastPageIndex);
}

const QImage &MangaView::image() const { return image_; }

const QVector<QImage> &MangaView::pageImages() const { return pageImages_; }

int MangaView::pageCount() const { return pageCount_; }

int MangaView::currentPageIndex() const { return currentPageIndex_; }

ViewMode MangaView::viewMode() const { return viewMode_; }

ReadingDirection MangaView::readingDirection() const { return readingDirection_; }

ViewerState MangaView::viewerState() const {
    return {
        currentPageIndex_, currentDisplayLastPageIndex_, viewMode_, readingDirection_, spreadGroupDirection_,
    };
}

void MangaView::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);

    const auto pageIndices = paintPageIndices();
    if (pageIndices.isEmpty()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (pageIndices.size() == 1) {
        const auto &pageImage = pageImages_.at(pageIndices.first());
        painter.drawImage(fittedImageRect(rect(), pageImage), pageImage);
    } else {
        const auto halfWidth = width() / 2.0;
        const QRectF leftRect(0, 0, halfWidth, height());
        const QRectF rightRect(halfWidth, 0, width() - halfWidth, height());
        const auto &firstPageImage = pageImages_.at(pageIndices.at(0));
        const auto &secondPageImage = pageImages_.at(pageIndices.at(1));
        const auto *leftPageImage =
            readingDirection_ == ReadingDirection::RightToLeft ? &secondPageImage : &firstPageImage;
        const auto *rightPageImage =
            readingDirection_ == ReadingDirection::RightToLeft ? &firstPageImage : &secondPageImage;

        painter.drawImage(fittedImageRect(leftRect, *leftPageImage, Qt::AlignRight), *leftPageImage);
        painter.drawImage(fittedImageRect(rightRect, *rightPageImage, Qt::AlignLeft), *rightPageImage);
    }

    const auto watermarkText = pendingPageWatermarkText();
    if (!watermarkText.isEmpty()) {
        auto watermarkFont = painter.font();
        watermarkFont.setBold(true);
        watermarkFont.setPixelSize(std::clamp(height() / 10, 32, 72));
        painter.setFont(watermarkFont);
        painter.setPen(QColor(0, 0, 0, 140));
        painter.drawText(rect().translated(2, 2), Qt::AlignCenter, watermarkText);
        painter.setPen(QColor(255, 255, 255, 170));
        painter.drawText(rect(), Qt::AlignCenter, watermarkText);
    }
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
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    setFocus(Qt::MouseFocusReason);

    if (event->button() == Qt::LeftButton) {
        goToDirectionAwareLeft();
    } else {
        goToDirectionAwareRight();
    }
    event->accept();
}

void MangaView::wheelEvent(QWheelEvent *event) {
    const auto verticalDelta = event->angleDelta().y() != 0 ? event->angleDelta().y() : event->pixelDelta().y();
    if (verticalDelta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    setFocus(Qt::MouseFocusReason);

    if (verticalDelta < 0) {
        goToNextPage();
    } else {
        goToPreviousPage();
    }
    event->accept();
}

QVector<int> MangaView::displayPageIndices() const { return currentDisplayPageIndices_; }

QVector<int> MangaView::paintPageIndices() const {
    if (hasImagesForPages(currentDisplayPageIndices_)) {
        return currentDisplayPageIndices_;
    }

    return hasImagesForPages(lastPaintablePageIndices_) ? lastPaintablePageIndices_ : QVector<int>{};
}

QString MangaView::pendingPageWatermarkText() const {
    if (currentDisplayPageIndices_.isEmpty() || hasImagesForPages(currentDisplayPageIndices_) ||
        !hasImagesForPages(lastPaintablePageIndices_)) {
        return {};
    }

    if (currentDisplayPageIndices_.size() == 1) {
        return QStringLiteral("%1 / %2").arg(currentDisplayPageIndices_.first() + 1).arg(pageCount_);
    }

    return QStringLiteral("%1-%2 / %3")
        .arg(currentDisplayPageIndices_.first() + 1)
        .arg(currentDisplayPageIndices_.last() + 1)
        .arg(pageCount_);
}

QVector<int> MangaView::forwardSpreadGroup(int pageIndex) const {
    if (!hasPages()) {
        return {};
    }

    const auto firstPageIndex = clampedPageIndex(pageIndex);
    if (!hasPage(firstPageIndex)) {
        return {};
    }

    if (viewMode_ == ViewMode::SinglePage || isLandscapePage(firstPageIndex)) {
        return {firstPageIndex};
    }

    const auto nextPageIndex = firstPageIndex + 1;
    if (!hasPage(nextPageIndex) || isLandscapePage(nextPageIndex)) {
        return {firstPageIndex};
    }

    return {firstPageIndex, nextPageIndex};
}

QVector<int> MangaView::backwardSpreadGroup(int pageIndex) const {
    if (!hasPages()) {
        return {};
    }

    const auto lastPageIndex = clampedPageIndex(pageIndex);
    if (!hasPage(lastPageIndex)) {
        return {};
    }

    if (viewMode_ == ViewMode::SinglePage || isLandscapePage(lastPageIndex)) {
        return {lastPageIndex};
    }

    const auto previousPageIndex = lastPageIndex - 1;
    if (!hasPage(previousPageIndex) || isLandscapePage(previousPageIndex)) {
        return {lastPageIndex};
    }

    return {previousPageIndex, lastPageIndex};
}

QRectF MangaView::fittedImageRect(const QRectF &availableRect, const QImage &image,
                                  Qt::Alignment horizontalAlignment) const {
    const auto availableSize = availableRect.size();
    const auto imageSize = QSizeF(image.size());

    if (availableSize.isEmpty() || imageSize.isEmpty()) {
        return {};
    }

    const auto scale = std::min(availableSize.width() / imageSize.width(), availableSize.height() / imageSize.height());
    const auto fittedSize = imageSize * scale;
    auto left = availableRect.left() + (availableSize.width() - fittedSize.width()) / 2.0;
    if (horizontalAlignment.testFlag(Qt::AlignLeft)) {
        left = availableRect.left();
    } else if (horizontalAlignment.testFlag(Qt::AlignRight)) {
        left = availableRect.right() - fittedSize.width();
    }
    const auto topLeft = QPointF(left, availableRect.top() + (availableSize.height() - fittedSize.height()) / 2.0);

    return QRectF(topLeft, fittedSize);
}

int MangaView::clampedPageIndex(int pageIndex) const {
    if (!hasPages()) {
        return 0;
    }
    return std::clamp(pageIndex, 0, pageCount_ - 1);
}

bool MangaView::hasPages() const { return pageCount_ > 0; }

bool MangaView::hasPage(int pageIndex) const { return pageIndex >= 0 && pageIndex < pageCount_; }

bool MangaView::hasImageForPage(int pageIndex) const {
    return pageIndex >= 0 && pageIndex < pageImages_.size() && !pageImages_.at(pageIndex).isNull();
}

bool MangaView::hasImagesForPages(const QVector<int> &pageIndices) const {
    if (pageIndices.isEmpty()) {
        return false;
    }

    for (const auto pageIndex : pageIndices) {
        if (!hasImageForPage(pageIndex)) {
            return false;
        }
    }
    return true;
}

bool MangaView::isLandscapePage(int pageIndex) const {
    if (!hasPage(pageIndex)) {
        return false;
    }

    if (pageIndex < landscapePages_.size()) {
        return landscapePages_.at(pageIndex);
    }

    return false;
}

void MangaView::goToFirstPage() { setCurrentPageIndex(0); }

void MangaView::goToLastPage() { setCurrentPageIndex(pageCount_ - 1); }

void MangaView::goToNextPage() {
    if (viewMode_ != ViewMode::Spread) {
        setCurrentPageIndex(currentPageIndex_ + 1);
        return;
    }

    const auto pageIndices = displayPageIndices();
    const auto nextPageIndex = pageIndices.isEmpty() ? currentPageIndex_ + 1 : pageIndices.last() + 1;
    if (nextPageIndex >= pageCount_) {
        return;
    }

    setCurrentPageIndexFromGroup(forwardSpreadGroup(nextPageIndex), nextPageIndex, SpreadGroupDirection::Forward);
}

void MangaView::goToPreviousPage() {
    if (viewMode_ != ViewMode::Spread) {
        setCurrentPageIndex(currentPageIndex_ - 1);
        return;
    }

    const auto pageIndices = displayPageIndices();
    const auto previousPageIndex = (pageIndices.isEmpty() ? currentPageIndex_ : pageIndices.first()) - 1;
    if (previousPageIndex < 0) {
        return;
    }

    setCurrentPageIndexFromGroup(backwardSpreadGroup(previousPageIndex), previousPageIndex,
                                 SpreadGroupDirection::Backward);
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
