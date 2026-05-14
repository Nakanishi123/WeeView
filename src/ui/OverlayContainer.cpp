#include "OverlayContainer.h"

#include "ui/FooterBar.h"
#include "ui/HeaderBar.h"
#include "ui/MangaView.h"
#include "ui/Sidebar.h"

#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimer>

namespace weeview {
namespace {

constexpr int edgeTriggerSize = 24;
constexpr int overlayHideDelayMs = 800;

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
    headerHideTimer_->setInterval(overlayHideDelayMs);
    footerHideTimer_->setSingleShot(true);
    footerHideTimer_->setInterval(overlayHideDelayMs);
    sidebarHideTimer_->setSingleShot(true);
    sidebarHideTimer_->setInterval(overlayHideDelayMs);

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

bool OverlayContainer::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (auto *widget = qobject_cast<QWidget *>(watched)) {
            handleMousePosition(widget->mapTo(this, mouseEvent->position().toPoint()));
        }
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
    if (position.y() <= edgeTriggerSize) {
        showHeader();
    } else if (headerBar_->isVisible() && isHeaderActive(position)) {
        headerHideTimer_->stop();
    } else if (headerBar_->isVisible() && !isHeaderActive(position)) {
        scheduleHeaderHide();
    }

    if (position.y() >= height() - edgeTriggerSize) {
        showFooter();
    } else if (footerBar_->isVisible() && isFooterActive(position)) {
        footerHideTimer_->stop();
    } else if (footerBar_->isVisible() && !isFooterActive(position)) {
        scheduleFooterHide();
    }

    if (position.x() <= edgeTriggerSize) {
        showSidebar();
    } else if (sidebar_->isVisible() && isSidebarActive(position)) {
        sidebarHideTimer_->stop();
    } else if (sidebar_->isVisible() && !isSidebarActive(position)) {
        scheduleSidebarHide();
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
    return position.y() <= edgeTriggerSize || headerBar_->geometry().contains(position);
}

bool OverlayContainer::isFooterActive(const QPoint &position) const {
    return position.y() >= height() - edgeTriggerSize || footerBar_->geometry().contains(position);
}

bool OverlayContainer::isSidebarActive(const QPoint &position) const {
    return position.x() <= edgeTriggerSize || sidebar_->geometry().contains(position);
}

} // namespace weeview
