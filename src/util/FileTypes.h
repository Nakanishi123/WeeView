#pragma once

#include <QString>
#include <QStringView>

namespace weeview::filetypes {

bool isSupportedImageExtension(QStringView extension);
bool isSupportedArchiveExtension(QStringView extension);
bool isSupportedImageFile(const QString &path);
bool isSupportedArchiveFile(const QString &path);

} // namespace weeview::filetypes
