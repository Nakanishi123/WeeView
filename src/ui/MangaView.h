#pragma once

#include "model/CoreTypes.h"

#include <QImage>
#include <QRectF>
#include <QSet>
#include <QVector>
#include <QWidget>

namespace weeview {

class MangaView final : public QWidget {
    Q_OBJECT

  public:
    explicit MangaView(QWidget *parent = nullptr);

    void setImage(QImage image);
    void setPageImage(int pageIndex, QImage image);
    void setPageImages(QVector<QImage> images);
    void clearImage();
    void clearPageImages();
    void retainPageImages(const QSet<int> &pageIndices);
    void setPageCount(int pageCount);
    void setCurrentPageIndex(int pageIndex);
    void setViewMode(ViewMode viewMode);
    void setReadingDirection(ReadingDirection readingDirection);
    void setViewerState(const ViewerState &state);

    [[nodiscard]] const QImage &image() const;
    [[nodiscard]] const QVector<QImage> &pageImages() const;
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
    void wheelEvent(QWheelEvent *event) override;

  private:
    [[nodiscard]] QVector<int> displayPageIndices() const;
    [[nodiscard]] QVector<int> forwardSpreadGroup(int pageIndex) const;
    [[nodiscard]] QVector<int> backwardSpreadGroup(int pageIndex) const;
    [[nodiscard]] QRectF fittedImageRect(const QRectF &availableRect, const QImage &image,
                                         Qt::Alignment horizontalAlignment = Qt::AlignHCenter) const;
    [[nodiscard]] int clampedPageIndex(int pageIndex) const;
    [[nodiscard]] bool hasPages() const;
    [[nodiscard]] bool hasImageForPage(int pageIndex) const;
    [[nodiscard]] bool isLandscapePage(int pageIndex) const;
    void setCurrentPageIndexFromGroup(QVector<int> pageIndices, int fallbackPageIndex, SpreadGroupDirection direction,
                                      int displayLastFallbackPageIndex = -1);
    void refreshCurrentDisplayGroup();
    void goToFirstPage();
    void goToLastPage();
    void goToNextPage();
    void goToPreviousPage();
    void goToDirectionAwareLeft();
    void goToDirectionAwareRight();

    QImage image_;
    QVector<QImage> pageImages_;
    QVector<int> currentDisplayPageIndices_;
    int currentDisplayLastPageIndex_ = 0;
    int pageCount_ = 0;
    int currentPageIndex_ = 0;
    ViewMode viewMode_ = ViewMode::SinglePage;
    ReadingDirection readingDirection_ = ReadingDirection::RightToLeft;
    SpreadGroupDirection spreadGroupDirection_ = SpreadGroupDirection::Forward;
};

} // namespace weeview
