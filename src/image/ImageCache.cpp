#include "ImageCache.h"

#include <algorithm>
#include <utility>

namespace weeview {

ImageCache::ImageCache(int maxPages) : maxPages_(std::max(1, maxPages)) {}

void ImageCache::clear() {
    images_.clear();
    usageOrder_.clear();
}

void ImageCache::insert(int pageIndex, QImage image) {
    if (pageIndex < 0 || image.isNull()) {
        return;
    }

    images_.insert(pageIndex, std::move(image));
    touch(pageIndex);
    evictOverflow();
}

bool ImageCache::contains(int pageIndex) const { return images_.contains(pageIndex); }

QImage ImageCache::image(int pageIndex) const { return images_.value(pageIndex); }

int ImageCache::size() const { return images_.size(); }

int ImageCache::maxPages() const { return maxPages_; }

void ImageCache::touch(int pageIndex) {
    usageOrder_.removeAll(pageIndex);
    usageOrder_.append(pageIndex);
}

void ImageCache::evictOverflow() {
    while (images_.size() > maxPages_ && !usageOrder_.isEmpty()) {
        images_.remove(usageOrder_.takeFirst());
    }
}

} // namespace weeview
