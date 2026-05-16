#pragma once

#include "archive/ArchiveReader.h"

#include <QHash>
#include <QStringList>

#include <memory>

struct archive;

namespace weeview {

class SevenZipArchiveReader final : public ArchiveReader {
  public:
    explicit SevenZipArchiveReader(QString archivePath);
    ~SevenZipArchiveReader() override;

    SevenZipArchiveReader(const SevenZipArchiveReader &) = delete;
    SevenZipArchiveReader &operator=(const SevenZipArchiveReader &) = delete;
    SevenZipArchiveReader(SevenZipArchiveReader &&) noexcept;
    SevenZipArchiveReader &operator=(SevenZipArchiveReader &&) noexcept;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] QVector<ArchiveEntry> entries() const override;
    [[nodiscard]] QByteArray readFile(const QString &entryPath) const override;

  private:
    struct ArchiveDeleter {
        void operator()(archive *archiveReader) const;
    };

    [[nodiscard]] std::unique_ptr<archive, ArchiveDeleter> openArchive() const;
    [[nodiscard]] QByteArray cachedFile(const QString &entryPath) const;
    void cacheFile(const QString &entryPath, const QByteArray &data) const;

    QString archivePath_;
    mutable QHash<QString, QByteArray> extractedFileCache_;
    mutable QStringList extractedFileCacheOrder_;
    mutable qsizetype extractedFileCacheBytes_ = 0;
    bool canOpen_ = false;
};

} // namespace weeview
