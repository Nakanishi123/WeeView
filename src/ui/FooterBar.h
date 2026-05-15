#pragma once

#include "model/CoreTypes.h"

#include <QWidget>

class QLabel;
class QSlider;

namespace weeview {

class FooterBar final : public QWidget {
    Q_OBJECT

  public:
    explicit FooterBar(QWidget *parent = nullptr);

    void setPageCount(int pageCount);
    void setCurrentPageIndex(int pageIndex);
    void setReadingDirection(ReadingDirection readingDirection);

    [[nodiscard]] int pageCount() const;
    [[nodiscard]] int currentPageIndex() const;
    [[nodiscard]] ReadingDirection readingDirection() const;

  signals:
    void currentPageIndexChanged(int pageIndex);

  private:
    void updatePageControls();

    QLabel *pageCounterLabel_ = nullptr;
    QSlider *pageSlider_ = nullptr;
    int pageCount_ = 0;
    int currentPageIndex_ = 0;
    ReadingDirection readingDirection_ = ReadingDirection::RightToLeft;
};

} // namespace weeview
