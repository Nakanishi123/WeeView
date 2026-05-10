#pragma once

#include <QHash>
#include <QImage>
#include <QList>

namespace weeview {

class ImageCache {
  public:
    explicit ImageCache(int maxPages = 8);

    void clear();
    void insert(int pageIndex, QImage image);

    [[nodiscard]] bool contains(int pageIndex) const;
    [[nodiscard]] QImage image(int pageIndex) const;
    [[nodiscard]] int size() const;
    [[nodiscard]] int maxPages() const;

  private:
    void touch(int pageIndex);
    void evictOverflow();

    int maxPages_ = 8;
    QHash<int, QImage> images_;
    QList<int> usageOrder_;
};

} // namespace weeview
