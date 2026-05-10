#pragma once

#include <QWidget>

class QTimer;

namespace weeview {

class FooterBar;
class HeaderBar;
class MangaView;

class OverlayContainer final : public QWidget {
    Q_OBJECT

  public:
    explicit OverlayContainer(QWidget *parent = nullptr);

    [[nodiscard]] MangaView *viewer() const;
    [[nodiscard]] HeaderBar *headerBar() const;
    [[nodiscard]] FooterBar *footerBar() const;

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void wireControls();
    void updateOverlayGeometry();
    void handleMousePosition(const QPoint &position);
    void showHeader();
    void showFooter();
    void scheduleHeaderHide();
    void scheduleFooterHide();
    [[nodiscard]] bool isHeaderActive(const QPoint &position) const;
    [[nodiscard]] bool isFooterActive(const QPoint &position) const;

    MangaView *viewer_ = nullptr;
    HeaderBar *headerBar_ = nullptr;
    FooterBar *footerBar_ = nullptr;
    QTimer *headerHideTimer_ = nullptr;
    QTimer *footerHideTimer_ = nullptr;
};

} // namespace weeview
