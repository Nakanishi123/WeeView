#pragma once

#include "model/CoreTypes.h"

#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QVector>
#include <QWidget>

class QPainter;

namespace weeview {

class MangaView final : public QWidget {
    Q_OBJECT

  public:
    explicit MangaView(QWidget *parent = nullptr);

    void setImage(QImage image);
    void setPageImage(int pageIndex, QImage image);
    void setPageImages(QVector<QImage> images);
    void setPageLandscapeFlags(QVector<bool> landscapePages);
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
    void previousBookRequested();
    void nextBookRequested();

  protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

  private:
    enum class GestureDirection {
        Left,
        Right,
        Up,
        Down,
    };

    enum class GestureAction {
        NextSinglePageStep,
        PreviousSinglePageStep,
        FirstPage,
        LastPage,
        PreviousBook,
        NextBook,
    };

    struct GestureCommand {
        QVector<GestureDirection> directions;
        QString text;
        GestureAction action;
    };

    [[nodiscard]] static const QVector<GestureCommand> &rightButtonGestureCommands();
    [[nodiscard]] QVector<int> displayPageIndices() const;
    [[nodiscard]] QVector<int> paintPageIndices() const;
    [[nodiscard]] QString pendingPageWatermarkText() const;
    [[nodiscard]] QString rightButtonGestureWatermarkText() const;
    [[nodiscard]] QString rightButtonGestureArrowText() const;
    [[nodiscard]] const GestureCommand *matchingRightButtonGestureCommand() const;
    [[nodiscard]] bool isRightButtonGestureCommand() const;
    [[nodiscard]] QVector<int> forwardSpreadGroup(int pageIndex) const;
    [[nodiscard]] QVector<int> backwardSpreadGroup(int pageIndex) const;
    [[nodiscard]] QRectF fittedImageRect(const QRectF &availableRect, const QImage &image,
                                         Qt::Alignment horizontalAlignment = Qt::AlignHCenter) const;
    [[nodiscard]] int clampedPageIndex(int pageIndex) const;
    [[nodiscard]] bool hasPages() const;
    [[nodiscard]] bool hasPage(int pageIndex) const;
    [[nodiscard]] bool hasImageForPage(int pageIndex) const;
    [[nodiscard]] bool hasImagesForPages(const QVector<int> &pageIndices) const;
    [[nodiscard]] bool isLandscapePage(int pageIndex) const;
    void setCurrentPageIndexFromGroup(QVector<int> pageIndices, int fallbackPageIndex, SpreadGroupDirection direction,
                                      int displayLastFallbackPageIndex = -1);
    void drawPageLoadWatermark(QPainter &painter) const;
    void drawRightButtonGestureWatermark(QPainter &painter) const;
    void refreshCurrentDisplayGroup();
    void goToFirstPage();
    void goToLastPage();
    void goToNextPage();
    void goToPreviousPage();
    void goToNextSinglePageStep();
    void goToPreviousSinglePageStep();
    void goToDirectionAwareLeft();
    void goToDirectionAwareRight();
    void appendRightButtonGestureDirection(GestureDirection direction, const QPointF &position);
    void executeRightButtonGestureCommand();

    QImage image_;
    QVector<QImage> pageImages_;
    QVector<bool> landscapePages_;
    QVector<int> currentDisplayPageIndices_;
    QVector<int> lastPaintablePageIndices_;
    int currentDisplayLastPageIndex_ = 0;
    int pageCount_ = 0;
    int currentPageIndex_ = 0;
    bool rightButtonPressed_ = false;
    bool rightButtonGestureAmbiguous_ = false;
    QPointF rightButtonGestureAnchorPosition_;
    QVector<GestureDirection> rightButtonGestureDirections_;
    ViewMode viewMode_ = ViewMode::SinglePage;
    ReadingDirection readingDirection_ = ReadingDirection::RightToLeft;
    SpreadGroupDirection spreadGroupDirection_ = SpreadGroupDirection::Forward;
};

} // namespace weeview
