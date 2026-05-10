#pragma once

#include "archive/ArchiveReader.h"

#include <memory>

struct zip;

namespace weeview {

class ZipArchiveReader final : public ArchiveReader {
  public:
    explicit ZipArchiveReader(QString archivePath);
    ~ZipArchiveReader() override;

    ZipArchiveReader(const ZipArchiveReader &) = delete;
    ZipArchiveReader &operator=(const ZipArchiveReader &) = delete;
    ZipArchiveReader(ZipArchiveReader &&) noexcept;
    ZipArchiveReader &operator=(ZipArchiveReader &&) noexcept;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] QVector<ArchiveEntry> entries() const override;
    [[nodiscard]] QByteArray readFile(const QString &entryPath) const override;

  private:
    struct ZipDeleter {
        void operator()(zip *archive) const;
    };

    [[nodiscard]] QString entryName(qsizetype entryIndex) const;

    QString archivePath_;
    std::unique_ptr<zip, ZipDeleter> archive_;
};

} // namespace weeview
