#include "FileTypes.h"

#include <QFileInfo>
#include <QStringList>

namespace weeview::filetypes {
namespace {

QString normalizedExtension(QStringView extension) {
    auto normalized = extension.toString().trimmed().toLower();
    while (normalized.startsWith('.')) {
        normalized.remove(0, 1);
    }
    return normalized;
}

bool containsExtension(const QStringList &extensions, QStringView extension) {
    return extensions.contains(normalizedExtension(extension));
}

} // namespace

bool isSupportedImageExtension(QStringView extension) {
    static const QStringList extensions = {"jpg", "jpeg", "png", "webp",
                                           "avif"};
    return containsExtension(extensions, extension);
}

bool isSupportedArchiveExtension(QStringView extension) {
    static const QStringList extensions = {"zip", "cbz"};
    return containsExtension(extensions, extension);
}

bool isSupportedImageFile(const QString &path) {
    return isSupportedImageExtension(QFileInfo(path).suffix());
}

bool isSupportedArchiveFile(const QString &path) {
    return isSupportedArchiveExtension(QFileInfo(path).suffix());
}

} // namespace weeview::filetypes
