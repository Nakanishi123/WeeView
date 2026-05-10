#pragma once

#include <QImage>
#include <QRectF>
#include <QWidget>

namespace weeview {

class MangaView final : public QWidget {
  public:
    explicit MangaView(QWidget *parent = nullptr);

    void setImage(QImage image);
    void clearImage();

    [[nodiscard]] const QImage &image() const;

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    [[nodiscard]] QRectF fittedImageRect() const;

    QImage image_;
};

} // namespace weeview
