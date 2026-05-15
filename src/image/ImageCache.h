#pragma once

#include <QHash>
#include <QImage>
#include <QList>
#include <QSet>

namespace weeview {

class ImageCache {
  public:
    explicit ImageCache(qint64 maxBytes = 256LL * 1024LL * 1024LL);

    void clear();
    void insert(int pageIndex, QImage image);
    void retain(const QSet<int> &pageIndices);
    void trimToMemoryLimit(const QSet<int> &protectedPageIndices = {});
    void setMaxBytes(qint64 maxBytes);

    [[nodiscard]] bool contains(int pageIndex) const;
    [[nodiscard]] QImage image(int pageIndex) const;
    [[nodiscard]] qint64 imageSizeInBytes(int pageIndex) const;
    [[nodiscard]] int size() const;
    [[nodiscard]] qint64 currentBytes() const;
    [[nodiscard]] qint64 maxBytes() const;

  private:
    void touch(int pageIndex);
    void remove(int pageIndex);

    qint64 maxBytes_ = 256LL * 1024LL * 1024LL;
    qint64 currentBytes_ = 0;
    QHash<int, QImage> images_;
    QList<int> usageOrder_;
};

} // namespace weeview
