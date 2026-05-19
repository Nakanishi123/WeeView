#include "MangaView.h"

#include <QBrush>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QStringList>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <utility>

namespace weeview {

namespace {
constexpr auto RightButtonGestureThreshold = 40.0;
constexpr auto RightButtonGestureDominanceRatio = 1.25;
constexpr int checkerboardCellSize = 12;
const QColor checkerboardBlack(0, 0, 0);
const QColor checkerboardDarkGray(24, 24, 24);

const QBrush &checkerboardBrush() {
    static const QBrush brush = [] {
        QPixmap pattern(checkerboardCellSize * 2, checkerboardCellSize * 2);
        pattern.fill(checkerboardBlack);

        QPainter painter(&pattern);
        painter.fillRect(0, 0, checkerboardCellSize, checkerboardCellSize, checkerboardDarkGray);
        painter.fillRect(checkerboardCellSize, checkerboardCellSize, checkerboardCellSize, checkerboardCellSize,
                         checkerboardDarkGray);
        return QBrush(pattern);
    }();
    return brush;
}
} // namespace

MangaView::MangaView(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
}

const QVector<MangaView::GestureCommand> &MangaView::rightButtonGestureCommands() {
    static const QVector<GestureCommand> commands{
        {{GestureDirection::Right, GestureDirection::Left},
         QStringLiteral("1ページ進む"),
         GestureAction::NextSinglePageStep},
        {{GestureDirection::Left, GestureDirection::Right},
         QStringLiteral("1ページ戻る"),
         GestureAction::PreviousSinglePageStep},
        {{GestureDirection::Up, GestureDirection::Right},
         QStringLiteral("最初のページへ移動"),
         GestureAction::FirstPage},
        {{GestureDirection::Up, GestureDirection::Left}, QStringLiteral("最後のページへ移動"), GestureAction::LastPage},
        {{GestureDirection::Down, GestureDirection::Right},
         QStringLiteral("前の本へ移動"),
         GestureAction::PreviousBook},
        {{GestureDirection::Down, GestureDirection::Left}, QStringLiteral("次の本へ移動"), GestureAction::NextBook},
    };
    return commands;
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
    auto retainedPageIndices = pageIndices;
    for (const auto pageIndex : lastPaintablePageIndices_) {
        retainedPageIndices.insert(pageIndex);
    }

    for (int index = 0; index < pageImages_.size(); ++index) {
        if (!retainedPageIndices.contains(index)) {
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

    QPainter painter(this);
    painter.fillRect(event->rect(), checkerboardBrush());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const auto pageIndices = paintPageIndices();
    if (pageIndices.isEmpty()) {
        drawRightButtonGestureWatermark(painter);
        return;
    }

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

    drawPageLoadWatermark(painter);
    drawRightButtonGestureWatermark(painter);
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
        rightButtonPressed_ = true;
        rightButtonGestureAmbiguous_ = false;
        rightButtonGestureDirections_.clear();
        rightButtonGestureAnchorPosition_ = event->position();
        update();
    }
    event->accept();
}

void MangaView::mouseMoveEvent(QMouseEvent *event) {
    if (!rightButtonPressed_ || !event->buttons().testFlag(Qt::RightButton)) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const auto delta = event->position() - rightButtonGestureAnchorPosition_;
    const auto absX = std::abs(delta.x());
    const auto absY = std::abs(delta.y());
    if (std::max(absX, absY) < RightButtonGestureThreshold) {
        event->accept();
        return;
    }

    if (absX >= absY * RightButtonGestureDominanceRatio) {
        appendRightButtonGestureDirection(delta.x() > 0 ? GestureDirection::Right : GestureDirection::Left,
                                          event->position());
    } else if (absY >= absX * RightButtonGestureDominanceRatio) {
        appendRightButtonGestureDirection(delta.y() > 0 ? GestureDirection::Down : GestureDirection::Up,
                                          event->position());
    } else {
        rightButtonGestureAmbiguous_ = true;
        rightButtonGestureAnchorPosition_ = event->position();
        update();
    }

    event->accept();
}

void MangaView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::RightButton || !rightButtonPressed_) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    if (rightButtonGestureDirections_.isEmpty() && !rightButtonGestureAmbiguous_) {
        goToDirectionAwareRight();
    } else if (isRightButtonGestureCommand()) {
        executeRightButtonGestureCommand();
    }

    rightButtonPressed_ = false;
    rightButtonGestureAmbiguous_ = false;
    rightButtonGestureDirections_.clear();
    update();
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

QString MangaView::rightButtonGestureWatermarkText() const {
    if (!rightButtonPressed_) {
        return {};
    }

    const auto arrowText = rightButtonGestureArrowText();
    if (arrowText.isEmpty()) {
        return QStringLiteral("右ボタンジェスチャー");
    }

    const auto *command = matchingRightButtonGestureCommand();
    return command == nullptr ? arrowText : QStringLiteral("%1\n%2").arg(arrowText, command->text);
}

QString MangaView::rightButtonGestureArrowText() const {
    QStringList arrows;
    arrows.reserve(rightButtonGestureDirections_.size());

    for (const auto direction : rightButtonGestureDirections_) {
        switch (direction) {
        case GestureDirection::Left:
            arrows.append(QStringLiteral("←"));
            break;
        case GestureDirection::Right:
            arrows.append(QStringLiteral("→"));
            break;
        case GestureDirection::Up:
            arrows.append(QStringLiteral("↑"));
            break;
        case GestureDirection::Down:
            arrows.append(QStringLiteral("↓"));
            break;
        }
    }

    return arrows.join(QLatin1Char(' '));
}

const MangaView::GestureCommand *MangaView::matchingRightButtonGestureCommand() const {
    if (rightButtonGestureAmbiguous_) {
        return nullptr;
    }

    for (const auto &command : rightButtonGestureCommands()) {
        if (rightButtonGestureDirections_ == command.directions) {
            return &command;
        }
    }

    return nullptr;
}

bool MangaView::isRightButtonGestureCommand() const { return matchingRightButtonGestureCommand() != nullptr; }

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

void MangaView::drawPageLoadWatermark(QPainter &painter) const {
    const auto watermarkText = pendingPageWatermarkText();
    if (watermarkText.isEmpty()) {
        return;
    }

    auto watermarkFont = painter.font();
    watermarkFont.setBold(true);
    watermarkFont.setPixelSize(std::clamp(height() / 10, 32, 72));
    painter.setFont(watermarkFont);
    painter.setPen(QColor(0, 0, 0, 140));
    painter.drawText(rect().translated(2, 2), Qt::AlignCenter, watermarkText);
    painter.setPen(QColor(255, 255, 255, 170));
    painter.drawText(rect(), Qt::AlignCenter, watermarkText);
}

void MangaView::drawRightButtonGestureWatermark(QPainter &painter) const {
    const auto watermarkText = rightButtonGestureWatermarkText();
    if (watermarkText.isEmpty()) {
        return;
    }

    auto gestureFont = painter.font();
    gestureFont.setBold(true);
    gestureFont.setPixelSize(std::clamp(height() / 18, 24, 48));
    painter.setFont(gestureFont);

    const QFontMetrics metrics(gestureFont);
    const auto textRect = metrics.boundingRect(QRect(0, 0, width(), height()), Qt::AlignCenter, watermarkText);
    const auto paddingX = std::clamp(width() / 20, 28, 72);
    const auto paddingY = std::clamp(height() / 32, 20, 48);
    auto backgroundRect = QRectF(textRect).adjusted(-paddingX, -paddingY, paddingX, paddingY);
    backgroundRect.moveCenter(rect().center());

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 150));
    painter.drawRoundedRect(backgroundRect, 8.0, 8.0);

    painter.setPen(QColor(255, 255, 255, 210));
    painter.drawText(backgroundRect, Qt::AlignCenter, watermarkText);
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

void MangaView::goToNextSinglePageStep() {
    if (viewMode_ == ViewMode::Spread && displayPageIndices().size() == 1) {
        const auto expandedGroup = forwardSpreadGroup(currentPageIndex_);
        if (expandedGroup.size() > displayPageIndices().size()) {
            setCurrentPageIndexFromGroup(expandedGroup, currentPageIndex_, SpreadGroupDirection::Forward);
            return;
        }
    }

    setCurrentPageIndex(currentPageIndex_ + 1);
}

void MangaView::goToPreviousSinglePageStep() { setCurrentPageIndex(currentPageIndex_ - 1); }

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

void MangaView::appendRightButtonGestureDirection(GestureDirection direction, const QPointF &position) {
    if (rightButtonGestureDirections_.isEmpty() || rightButtonGestureDirections_.last() != direction) {
        rightButtonGestureDirections_.append(direction);
        update();
    }
    rightButtonGestureAnchorPosition_ = position;
}

void MangaView::executeRightButtonGestureCommand() {
    const auto *command = matchingRightButtonGestureCommand();
    if (command == nullptr) {
        return;
    }

    switch (command->action) {
    case GestureAction::NextSinglePageStep:
        goToNextSinglePageStep();
        break;
    case GestureAction::PreviousSinglePageStep:
        goToPreviousSinglePageStep();
        break;
    case GestureAction::FirstPage:
        goToFirstPage();
        break;
    case GestureAction::LastPage:
        goToLastPage();
        break;
    case GestureAction::PreviousBook:
        emit previousBookRequested();
        break;
    case GestureAction::NextBook:
        emit nextBookRequested();
        break;
    }
}

} // namespace weeview
