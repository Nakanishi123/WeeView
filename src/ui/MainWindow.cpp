#include "MainWindow.h"

#include "app/AppSettingsStore.h"
#include "model/FolderBook.h"
#include "model/ZipBook.h"
#include "ui/HeaderBar.h"
#include "ui/MangaView.h"
#include "ui/OverlayContainer.h"
#include "ui/Sidebar.h"

#include <QDir>
#include <QFileInfo>
#include <QVector>

namespace weeview {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("WeeView"));
    resize(960, 720);

    overlayContainer_ = new OverlayContainer(this);
    setCentralWidget(overlayContainer_);

    const auto settings = AppSettingsStore().load();
    overlayContainer_->sidebar()->setHomeFolder(settings.homeFolder);
    overlayContainer_->sidebar()->setCurrentFolder(settings.homeFolder);

    wireSidebar();
}

void MainWindow::wireSidebar() {
    auto *sidebar = overlayContainer_->sidebar();
    connect(sidebar, &Sidebar::folderBookRequested, this,
            [this](const QString &folderPath) { openFolderBook(folderPath); });
    connect(sidebar, &Sidebar::imageFileRequested, this, &MainWindow::openImageFile);
    connect(sidebar, &Sidebar::archiveBookRequested, this, &MainWindow::openArchiveBook);
}

void MainWindow::openFolderBook(const QString &folderPath, int requestedPageIndex) {
    auto book = std::make_unique<FolderBook>(folderPath);
    currentBook_ = std::move(book);

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    displayBook(requestedPageIndex);
}

void MainWindow::openImageFile(const QString &filePath) {
    const QFileInfo fileInfo(filePath);
    auto book = std::make_unique<FolderBook>(fileInfo.dir().absolutePath());
    currentBook_ = std::move(book);

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    displayBook(pageIndexForPath(fileInfo.absoluteFilePath()));
}

void MainWindow::openArchiveBook(const QString &archivePath) {
    auto book = std::make_unique<ZipBook>(archivePath);
    currentBook_ = std::move(book);

    const QFileInfo archiveInfo(archivePath);
    overlayContainer_->sidebar()->setCurrentFolder(archiveInfo.dir().absolutePath());
    overlayContainer_->sidebar()->setCurrentArchivePath(archiveInfo.absoluteFilePath());
    displayBook(0);
}

void MainWindow::displayBook(int currentPageIndex) {
    auto *viewer = overlayContainer_->viewer();
    auto *header = overlayContainer_->headerBar();

    if (!currentBook_ || currentBook_->pageCount() == 0) {
        viewer->clearPageImages();
        header->setBookPath({});
        return;
    }

    QVector<QImage> images;
    images.reserve(currentBook_->pageCount());
    for (int index = 0; index < currentBook_->pageCount(); ++index) {
        images.append(currentBook_->loadPage(index));
    }

    header->setBookPath(currentBook_->sourcePath());
    viewer->setPageImages(std::move(images));
    viewer->setCurrentPageIndex(currentPageIndex);
}

int MainWindow::pageIndexForPath(const QString &filePath) const {
    if (!currentBook_) {
        return 0;
    }

    const auto normalizedFilePath = QFileInfo(filePath).absoluteFilePath();
    for (int index = 0; index < currentBook_->pageCount(); ++index) {
        const auto page = currentBook_->pageInfo(index);
        if (QFileInfo(page.displayPath).absoluteFilePath() == normalizedFilePath) {
            return index;
        }
    }
    return 0;
}

} // namespace weeview
