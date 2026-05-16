#include "OverlayContainer.h"

#include "ui/FooterBar.h"
#include "ui/HeaderBar.h"
#include "ui/MangaView.h"
#include "ui/Sidebar.h"

#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>

#include <algorithm>

namespace weeview {

namespace {

constexpr int resizeBorderWidth = 6;

Qt::CursorShape cursorForResizeEdges(Qt::Edges edges) {
    const auto horizontal = edges & (Qt::LeftEdge | Qt::RightEdge);
    const auto vertical = edges & (Qt::TopEdge | Qt::BottomEdge);

    if (horizontal && vertical) {
        const auto topLeftOrBottomRight = (edges.testFlag(Qt::LeftEdge) && edges.testFlag(Qt::TopEdge)) ||
                                          (edges.testFlag(Qt::RightEdge) && edges.testFlag(Qt::BottomEdge));
        return topLeftOrBottomRight ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
    }
    if (horizontal) {
        return Qt::SizeHorCursor;
    }
    if (vertical) {
        return Qt::SizeVerCursor;
    }

    return Qt::ArrowCursor;
}

} // namespace

OverlayContainer::OverlayContainer(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);

    viewer_ = new MangaView(this);
    headerBar_ = new HeaderBar(this);
    footerBar_ = new FooterBar(this);
    sidebar_ = new Sidebar(this);
    headerHideTimer_ = new QTimer(this);
    footerHideTimer_ = new QTimer(this);
    sidebarHideTimer_ = new QTimer(this);

    headerHideTimer_->setSingleShot(true);
    headerHideTimer_->setInterval(800);
    footerHideTimer_->setSingleShot(true);
    footerHideTimer_->setInterval(800);
    sidebarHideTimer_->setSingleShot(true);
    sidebarHideTimer_->setInterval(800);

    headerBar_->hide();
    footerBar_->hide();
    sidebar_->hide();

    viewer_->installEventFilter(this);
    headerBar_->installEventFilter(this);
    footerBar_->installEventFilter(this);
    sidebar_->installEventFilter(this);
    viewer_->setMouseTracking(true);
    headerBar_->setMouseTracking(true);
    footerBar_->setMouseTracking(true);
    sidebar_->setMouseTracking(true);

    connect(headerHideTimer_, &QTimer::timeout, headerBar_, &QWidget::hide);
    connect(footerHideTimer_, &QTimer::timeout, footerBar_, &QWidget::hide);
    connect(sidebarHideTimer_, &QTimer::timeout, sidebar_, &QWidget::hide);
    connect(sidebar_, &Sidebar::sidebarWidthChanged, this, &OverlayContainer::updateOverlayGeometry);

    wireControls();
}

MangaView *OverlayContainer::viewer() const { return viewer_; }

HeaderBar *OverlayContainer::headerBar() const { return headerBar_; }

FooterBar *OverlayContainer::footerBar() const { return footerBar_; }

Sidebar *OverlayContainer::sidebar() const { return sidebar_; }

void OverlayContainer::setOverlaySettings(int edgeTriggerSize, int hideDelayMs) {
    edgeTriggerSize_ = std::max(1, edgeTriggerSize);
    const auto clampedHideDelayMs = std::max(0, hideDelayMs);
    headerHideTimer_->setInterval(clampedHideDelayMs);
    footerHideTimer_->setInterval(clampedHideDelayMs);
    sidebarHideTimer_->setInterval(clampedHideDelayMs);
}

bool OverlayContainer::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (auto *widget = qobject_cast<QWidget *>(watched)) {
            const auto position = widget->mapTo(this, mouseEvent->position().toPoint());
            updateResizeCursor(resizeEdgesAt(position));
            handleMousePosition(position);
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (auto *widget = qobject_cast<QWidget *>(watched)) {
                const auto edges = resizeEdgesAt(widget->mapTo(this, mouseEvent->position().toPoint()));
                if (edges != Qt::Edges{}) {
                    emit windowResizeRequested(edges);
                    return true;
                }
            }
        }
    } else if (event->type() == QEvent::Leave) {
        updateResizeCursor({});
    }

    return QWidget::eventFilter(watched, event);
}

void OverlayContainer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateOverlayGeometry();
}

void OverlayContainer::wireControls() {
    connect(headerBar_, &HeaderBar::viewModeChanged, viewer_, &MangaView::setViewMode);
    connect(headerBar_, &HeaderBar::readingDirectionChanged, viewer_, &MangaView::setReadingDirection);
    connect(viewer_, &MangaView::viewModeChanged, headerBar_, &HeaderBar::setViewMode);
    connect(viewer_, &MangaView::readingDirectionChanged, headerBar_, &HeaderBar::setReadingDirection);
    connect(viewer_, &MangaView::readingDirectionChanged, footerBar_, &FooterBar::setReadingDirection);

    connect(viewer_, &MangaView::pageCountChanged, footerBar_, &FooterBar::setPageCount);
    connect(viewer_, &MangaView::currentPageIndexChanged, footerBar_, &FooterBar::setCurrentPageIndex);
    connect(footerBar_, &FooterBar::currentPageIndexChanged, viewer_, &MangaView::setCurrentPageIndex);

    headerBar_->setViewMode(viewer_->viewMode());
    headerBar_->setReadingDirection(viewer_->readingDirection());
    footerBar_->setReadingDirection(viewer_->readingDirection());
    footerBar_->setPageCount(viewer_->pageCount());
    footerBar_->setCurrentPageIndex(viewer_->currentPageIndex());
}

void OverlayContainer::updateOverlayGeometry() {
    viewer_->setGeometry(rect());

    const auto headerHeight = headerBar_->sizeHint().height();
    const auto footerHeight = footerBar_->sizeHint().height();

    headerBar_->setGeometry(0, 0, width(), headerHeight);
    footerBar_->setGeometry(0, height() - footerHeight, width(), footerHeight);
    sidebar_->setGeometry(0, 0, sidebar_->width(), height());

    sidebar_->raise();
    headerBar_->raise();
    footerBar_->raise();
}

void OverlayContainer::handleMousePosition(const QPoint &position) {
    if (position.y() <= edgeTriggerSize_) {
        showHeader();
    } else if (headerBar_->isVisible() && isHeaderActive(position)) {
        headerHideTimer_->stop();
    } else if (headerBar_->isVisible() && !isHeaderActive(position)) {
        scheduleHeaderHide();
    }

    if (position.y() >= height() - edgeTriggerSize_) {
        showFooter();
    } else if (footerBar_->isVisible() && isFooterActive(position)) {
        footerHideTimer_->stop();
    } else if (footerBar_->isVisible() && !isFooterActive(position)) {
        scheduleFooterHide();
    }

    if (position.x() <= edgeTriggerSize_) {
        showSidebar();
    } else if (sidebar_->isVisible() && isSidebarActive(position)) {
        sidebarHideTimer_->stop();
    } else if (sidebar_->isVisible() && !isSidebarActive(position)) {
        scheduleSidebarHide();
    }
}

Qt::Edges OverlayContainer::resizeEdgesAt(const QPoint &position) const {
    if (window()->isMaximized()) {
        return {};
    }

    Qt::Edges edges;
    if (position.x() < resizeBorderWidth) {
        edges |= Qt::LeftEdge;
    } else if (position.x() >= width() - resizeBorderWidth) {
        edges |= Qt::RightEdge;
    }

    if (position.y() < resizeBorderWidth) {
        edges |= Qt::TopEdge;
    } else if (position.y() >= height() - resizeBorderWidth) {
        edges |= Qt::BottomEdge;
    }

    return edges;
}

void OverlayContainer::updateResizeCursor(Qt::Edges edges) {
    const auto cursorShape = cursorForResizeEdges(edges);
    if (resizeCursorShape_ == cursorShape) {
        return;
    }

    resizeCursorShape_ = cursorShape;
    if (cursorShape == Qt::ArrowCursor) {
        unsetCursor();
    } else {
        setCursor(cursorShape);
    }
}

void OverlayContainer::showHeader() {
    headerHideTimer_->stop();
    headerBar_->show();
    headerBar_->raise();
}

void OverlayContainer::showFooter() {
    footerHideTimer_->stop();
    footerBar_->show();
    footerBar_->raise();
}

void OverlayContainer::showSidebar() {
    sidebarHideTimer_->stop();
    sidebar_->show();
    sidebar_->raise();
}

void OverlayContainer::scheduleHeaderHide() {
    if (!headerHideTimer_->isActive()) {
        headerHideTimer_->start();
    }
}

void OverlayContainer::scheduleFooterHide() {
    if (!footerHideTimer_->isActive()) {
        footerHideTimer_->start();
    }
}

void OverlayContainer::scheduleSidebarHide() {
    if (!sidebarHideTimer_->isActive()) {
        sidebarHideTimer_->start();
    }
}

bool OverlayContainer::isHeaderActive(const QPoint &position) const {
    return position.y() <= edgeTriggerSize_ || headerBar_->geometry().contains(position);
}

bool OverlayContainer::isFooterActive(const QPoint &position) const {
    return position.y() >= height() - edgeTriggerSize_ || footerBar_->geometry().contains(position);
}

bool OverlayContainer::isSidebarActive(const QPoint &position) const {
    return position.x() <= edgeTriggerSize_ || sidebar_->geometry().contains(position);
}

} // namespace weeview
