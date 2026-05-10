#include "ImageDecoder.h"

#include <QBuffer>
#include <QFileInfo>
#include <QIODevice>
#include <QImageReader>

namespace weeview {
namespace {

QByteArray imageFormatFromPath(const QString &displayPath) {
    const auto suffix = QFileInfo(displayPath).suffix().toLower();
    return suffix.isEmpty() ? QByteArray() : suffix.toLatin1();
}

void prepareBufferAndReader(const QByteArray &data, const QString &displayPath, QBuffer &buffer, QImageReader &reader) {
    buffer.setData(data);
    buffer.open(QIODevice::ReadOnly);

    reader.setDevice(&buffer);
    const auto format = imageFormatFromPath(displayPath);
    if (!format.isEmpty()) {
        reader.setFormat(format);
    }
}

} // namespace

QImage ImageDecoder::readFile(const QString &filePath) const {
    QImageReader reader(filePath);
    return reader.read();
}

QSize ImageDecoder::imageSize(const QString &filePath) const {
    QImageReader reader(filePath);
    return reader.size();
}

QImage ImageDecoder::readData(const QByteArray &data, const QString &displayPath) const {
    QBuffer buffer;
    QImageReader reader;
    prepareBufferAndReader(data, displayPath, buffer, reader);
    return reader.read();
}

QSize ImageDecoder::imageSize(const QByteArray &data, const QString &displayPath) const {
    QBuffer buffer;
    QImageReader reader;
    prepareBufferAndReader(data, displayPath, buffer, reader);
    return reader.size();
}

bool ImageDecoder::supportsAvif() { return supportsFormat(QByteArrayLiteral("avif")); }

bool ImageDecoder::supportsFormat(const QByteArray &format) {
    const auto normalizedFormat = format.toLower();
    for (const auto &supportedFormat : QImageReader::supportedImageFormats()) {
        if (supportedFormat.toLower() == normalizedFormat) {
            return true;
        }
    }
    return false;
}

} // namespace weeview
