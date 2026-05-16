#pragma once

#include <QWidget>

class QTimer;

namespace weeview {

class FooterBar;
class HeaderBar;
class MangaView;
class Sidebar;

class OverlayContainer final : public QWidget {
    Q_OBJECT

  public:
    explicit OverlayContainer(QWidget *parent = nullptr);

    [[nodiscard]] MangaView *viewer() const;
    [[nodiscard]] HeaderBar *headerBar() const;
    [[nodiscard]] FooterBar *footerBar() const;
    [[nodiscard]] Sidebar *sidebar() const;
    void setOverlaySettings(int edgeTriggerSize, int hideDelayMs);

  signals:
    void windowResizeRequested(Qt::Edges edges);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void wireControls();
    void updateOverlayGeometry();
    void handleMousePosition(const QPoint &position);
    [[nodiscard]] Qt::Edges resizeEdgesAt(const QPoint &position) const;
    void updateResizeCursor(Qt::Edges edges);
    void showHeader();
    void showFooter();
    void showSidebar();
    void scheduleHeaderHide();
    void scheduleFooterHide();
    void scheduleSidebarHide();
    [[nodiscard]] bool isHeaderActive(const QPoint &position) const;
    [[nodiscard]] bool isFooterActive(const QPoint &position) const;
    [[nodiscard]] bool isSidebarActive(const QPoint &position) const;

    MangaView *viewer_ = nullptr;
    HeaderBar *headerBar_ = nullptr;
    FooterBar *footerBar_ = nullptr;
    Sidebar *sidebar_ = nullptr;
    QTimer *headerHideTimer_ = nullptr;
    QTimer *footerHideTimer_ = nullptr;
    QTimer *sidebarHideTimer_ = nullptr;
    int edgeTriggerSize_ = 24;
    Qt::CursorShape resizeCursorShape_ = Qt::ArrowCursor;
};

} // namespace weeview
