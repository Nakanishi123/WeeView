#pragma once

#include "archive/ArchiveReader.h"

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

    QString archivePath_;
    bool canOpen_ = false;
};

} // namespace weeview
