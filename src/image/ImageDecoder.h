#pragma once

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>

namespace weeview {

class ImageDecoder {
  public:
    [[nodiscard]] QImage readFile(const QString &filePath) const;
    [[nodiscard]] QSize imageSize(const QString &filePath) const;
    [[nodiscard]] QImage readData(const QByteArray &data, const QString &displayPath = {}) const;
    [[nodiscard]] QSize imageSize(const QByteArray &data, const QString &displayPath = {}) const;

    [[nodiscard]] static bool supportsAvif();
    [[nodiscard]] static bool supportsFormat(const QByteArray &format);
};

} // namespace weeview
