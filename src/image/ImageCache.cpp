#include "ImageCache.h"

#include <algorithm>
#include <utility>

namespace weeview {

namespace {

qint64 imageBytes(const QImage &image) { return image.isNull() ? 0 : static_cast<qint64>(image.sizeInBytes()); }

} // namespace

ImageCache::ImageCache(qint64 maxBytes) : maxBytes_(std::max<qint64>(1, maxBytes)) {}

void ImageCache::clear() {
    images_.clear();
    usageOrder_.clear();
    currentBytes_ = 0;
}

void ImageCache::insert(int pageIndex, QImage image) {
    if (pageIndex < 0 || image.isNull()) {
        return;
    }

    if (images_.contains(pageIndex)) {
        currentBytes_ -= imageBytes(images_.value(pageIndex));
    }
    currentBytes_ += imageBytes(image);
    images_.insert(pageIndex, std::move(image));
    touch(pageIndex);
}

void ImageCache::retain(const QSet<int> &pageIndices) {
    const auto cachedPageIndices = images_.keys();
    for (const auto pageIndex : cachedPageIndices) {
        if (!pageIndices.contains(pageIndex)) {
            remove(pageIndex);
        }
    }
}

void ImageCache::trimToMemoryLimit(const QSet<int> &protectedPageIndices) {
    while (currentBytes_ > maxBytes_ && !usageOrder_.isEmpty()) {
        auto evicted = false;
        for (auto it = usageOrder_.begin(); it != usageOrder_.end(); ++it) {
            if (protectedPageIndices.contains(*it)) {
                continue;
            }

            const auto pageIndex = *it;
            usageOrder_.erase(it);
            currentBytes_ -= imageBytes(images_.value(pageIndex));
            images_.remove(pageIndex);
            evicted = true;
            break;
        }

        if (!evicted) {
            break;
        }
    }
}

void ImageCache::setMaxBytes(qint64 maxBytes) {
    maxBytes_ = std::max<qint64>(1, maxBytes);
    trimToMemoryLimit();
}

bool ImageCache::contains(int pageIndex) const { return images_.contains(pageIndex); }

QImage ImageCache::image(int pageIndex) const { return images_.value(pageIndex); }

qint64 ImageCache::imageSizeInBytes(int pageIndex) const { return imageBytes(images_.value(pageIndex)); }

int ImageCache::size() const { return images_.size(); }

qint64 ImageCache::currentBytes() const { return currentBytes_; }

qint64 ImageCache::maxBytes() const { return maxBytes_; }

void ImageCache::touch(int pageIndex) {
    usageOrder_.removeAll(pageIndex);
    usageOrder_.append(pageIndex);
}

void ImageCache::remove(int pageIndex) {
    if (!images_.contains(pageIndex)) {
        return;
    }

    currentBytes_ -= imageBytes(images_.value(pageIndex));
    images_.remove(pageIndex);
    usageOrder_.removeAll(pageIndex);
}

} // namespace weeview
