#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

namespace weeview {

struct ArchiveEntry {
    QString path;
    qsizetype size = 0;
};

class ArchiveReader {
  public:
    virtual ~ArchiveReader() = default;

    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual QVector<ArchiveEntry> entries() const = 0;
    [[nodiscard]] virtual QByteArray readFile(const QString &entryPath) const = 0;
};

} // namespace weeview
