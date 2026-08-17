#pragma once

#include "model/Book.h"

#include <QString>
#include <QVector>

namespace weeview {

class FolderBook final : public Book {
  public:
    explicit FolderBook(QString folderPath, SidebarSortSettings sortSettings = {});

    [[nodiscard]] BookType type() const override;
    [[nodiscard]] QString displayName() const override;
    [[nodiscard]] QString sourcePath() const override;
    [[nodiscard]] int pageCount() const override;
    [[nodiscard]] PageInfo pageInfo(int pageIndex) const override;
    [[nodiscard]] PageInfo loadPageInfo(int pageIndex) const override;
    [[nodiscard]] QImage loadPage(int pageIndex) const override;

  private:
    struct Page {
        QString filePath;
        PageInfo info;
    };

    void scanPages();
    [[nodiscard]] bool isValidPageIndex(int pageIndex) const;

    QString folderPath_;
    QString displayName_;
    SidebarSortSettings sortSettings_;
    QVector<Page> pages_;
};

} // namespace weeview
