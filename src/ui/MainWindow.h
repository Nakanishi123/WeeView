#pragma once

#include "model/Book.h"

#include <QMainWindow>
#include <memory>

namespace weeview {

class OverlayContainer;

class MainWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);

  private:
    void wireSidebar();
    void openFolderBook(const QString &folderPath, int requestedPageIndex = 0);
    void openImageFile(const QString &filePath);
    void openArchiveBook(const QString &archivePath);
    void displayBook(int currentPageIndex);
    [[nodiscard]] int pageIndexForPath(const QString &filePath) const;

    OverlayContainer *overlayContainer_ = nullptr;
    std::unique_ptr<Book> currentBook_;
};

} // namespace weeview
