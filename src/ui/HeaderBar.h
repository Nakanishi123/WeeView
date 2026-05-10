#pragma once

#include "model/CoreTypes.h"

#include <QWidget>

class QLabel;
class QPushButton;

namespace weeview {

class HeaderBar final : public QWidget {
    Q_OBJECT

  public:
    explicit HeaderBar(QWidget *parent = nullptr);

    void setBookPath(const QString &bookPath);
    void setViewMode(ViewMode viewMode);
    void setReadingDirection(ReadingDirection readingDirection);

    [[nodiscard]] QString bookPath() const;
    [[nodiscard]] ViewMode viewMode() const;
    [[nodiscard]] ReadingDirection readingDirection() const;

  signals:
    void viewModeChanged(ViewMode viewMode);
    void readingDirectionChanged(ReadingDirection readingDirection);

  private:
    void updateButtons();
    void toggleViewMode();
    void toggleReadingDirection();

    QLabel *bookPathLabel_ = nullptr;
    QPushButton *viewModeButton_ = nullptr;
    QPushButton *readingDirectionButton_ = nullptr;
    QString bookPath_;
    ViewMode viewMode_ = ViewMode::SinglePage;
    ReadingDirection readingDirection_ = ReadingDirection::RightToLeft;
};

} // namespace weeview
