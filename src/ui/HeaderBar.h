#pragma once

#include "model/CoreTypes.h"

#include <QWidget>

class QLabel;
class QMouseEvent;
class QPushButton;

namespace weeview {

class HeaderBar final : public QWidget {
    Q_OBJECT

  public:
    explicit HeaderBar(QWidget *parent = nullptr);

    void setBookPath(const QString &bookPath);
    void setViewMode(ViewMode viewMode);
    void setReadingDirection(ReadingDirection readingDirection);
    void setWindowMaximized(bool maximized);

    [[nodiscard]] QString bookPath() const;
    [[nodiscard]] ViewMode viewMode() const;
    [[nodiscard]] ReadingDirection readingDirection() const;

  signals:
    void viewModeChanged(ViewMode viewMode);
    void readingDirectionChanged(ReadingDirection readingDirection);
    void minimizeRequested();
    void maximizeRestoreRequested();
    void closeRequested();

  protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

  private:
    void updateButtons();
    void updateWindowButtons();
    void toggleViewMode();
    void toggleReadingDirection();

    QLabel *bookPathLabel_ = nullptr;
    QPushButton *viewModeButton_ = nullptr;
    QPushButton *readingDirectionButton_ = nullptr;
    QPushButton *minimizeButton_ = nullptr;
    QPushButton *maximizeRestoreButton_ = nullptr;
    QPushButton *closeButton_ = nullptr;
    QString bookPath_;
    ViewMode viewMode_ = ViewMode::SinglePage;
    ReadingDirection readingDirection_ = ReadingDirection::RightToLeft;
    bool windowMaximized_ = false;
};

} // namespace weeview
