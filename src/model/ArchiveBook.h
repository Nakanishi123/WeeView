#pragma once

#include "archive/ArchiveReader.h"
#include "model/Book.h"

#include <QString>
#include <QVector>

#include <memory>

namespace weeview {

class ArchiveBook final : public Book {
  public:
    explicit ArchiveBook(QString archivePath);
    ~ArchiveBook() override;

    ArchiveBook(const ArchiveBook &) = delete;
    ArchiveBook &operator=(const ArchiveBook &) = delete;
    ArchiveBook(ArchiveBook &&) noexcept;
    ArchiveBook &operator=(ArchiveBook &&) noexcept;

    [[nodiscard]] BookType type() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString sourcePath() const override;
    [[nodiscard]] int pageCount() const override;
    [[nodiscard]] PageInfo pageInfo(int pageIndex) const override;
    [[nodiscard]] PageInfo loadPageInfo(int pageIndex) const override;
    [[nodiscard]] QImage loadPage(int pageIndex) const override;

  private:
    struct Page {
        QString entryPath;
        PageInfo info;
    };

    void scanPages();
    [[nodiscard]] bool isValidPageIndex(int pageIndex) const;
    [[nodiscard]] QImage imageFromEntry(const QString &entryPath) const;

    QString archivePath_;
    QString displayName_;
    std::unique_ptr<ArchiveReader> reader_;
    QVector<Page> pages_;
};

} // namespace weeview
