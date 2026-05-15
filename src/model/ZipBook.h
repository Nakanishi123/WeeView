#pragma once

#include "archive/ZipArchiveReader.h"
#include "model/Book.h"

#include <QString>
#include <QVector>

namespace weeview {

class ZipBook final : public Book {
  public:
    explicit ZipBook(QString archivePath);

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
    ZipArchiveReader reader_;
    QVector<Page> pages_;
};

} // namespace weeview
