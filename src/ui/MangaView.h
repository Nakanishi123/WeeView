#pragma once

#include "model/CoreTypes.h"

#include <QImage>
#include <QRectF>
#include <QWidget>

namespace weeview {

class MangaView final : public QWidget {
    Q_OBJECT

  public:
    explicit MangaView(QWidget *parent = nullptr);

    void setImage(QImage image);
    void clearImage();
    void setPageCount(int pageCount);
    void setCurrentPageIndex(int pageIndex);
    void setViewMode(ViewMode viewMode);
    void setReadingDirection(ReadingDirection readingDirection);
    void setViewerState(const ViewerState &state);

    [[nodiscard]] const QImage &image() const;
    [[nodiscard]] int pageCount() const;
    [[nodiscard]] int currentPageIndex() const;
    [[nodiscard]] ViewMode viewMode() const;
    [[nodiscard]] ReadingDirection readingDirection() const;
    [[nodiscard]] ViewerState viewerState() const;

  signals:
    void currentPageIndexChanged(int pageIndex);
    void pageCountChanged(int pageCount);
    void viewModeChanged(ViewMode viewMode);
    void readingDirectionChanged(ReadingDirection readingDirection);

  protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

  private:
    [[nodiscard]] QRectF fittedImageRect() const;
    [[nodiscard]] int clampedPageIndex(int pageIndex) const;
    [[nodiscard]] bool hasPages() const;
    void goToFirstPage();
    void goToLastPage();
    void goToNextPage();
    void goToPreviousPage();
    void goToDirectionAwareLeft();
    void goToDirectionAwareRight();

    QImage image_;
    int pageCount_ = 0;
    int currentPageIndex_ = 0;
    ViewMode viewMode_ = ViewMode::SinglePage;
    ReadingDirection readingDirection_ = ReadingDirection::RightToLeft;
};

} // namespace weeview
