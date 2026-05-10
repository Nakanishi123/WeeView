#include "MainWindow.h"

#include "app/AppSettingsStore.h"
#include "model/FolderBook.h"
#include "model/ZipBook.h"
#include "ui/HeaderBar.h"
#include "ui/MangaView.h"
#include "ui/OverlayContainer.h"
#include "ui/Sidebar.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QVector>

#include <algorithm>

namespace weeview {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("WeeView"));
    resize(960, 720);

    overlayContainer_ = new OverlayContainer(this);
    setCentralWidget(overlayContainer_);

    const auto settings = AppSettingsStore().load();
    overlayContainer_->sidebar()->setHomeFolder(settings.homeFolder);
    overlayContainer_->sidebar()->setCurrentFolder(settings.homeFolder);

    loadHistory();
    wireSidebar();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveCurrentHistory();
    QMainWindow::closeEvent(event);
}

void MainWindow::wireSidebar() {
    auto *sidebar = overlayContainer_->sidebar();
    connect(sidebar, &Sidebar::folderBookRequested, this,
            [this](const QString &folderPath) { openFolderBook(folderPath); });
    connect(sidebar, &Sidebar::imageFileRequested, this, &MainWindow::openImageFile);
    connect(sidebar, &Sidebar::archiveBookRequested, this, &MainWindow::openArchiveBook);

    auto *viewer = overlayContainer_->viewer();
    connect(viewer, &MangaView::currentPageIndexChanged, this, &MainWindow::handleCurrentPageChanged);
    connect(viewer, &MangaView::viewModeChanged, this, [this] {
        refreshCachedImages();
        saveCurrentHistory();
    });
    connect(viewer, &MangaView::readingDirectionChanged, this, [this] { saveCurrentHistory(); });
}

void MainWindow::openFolderBook(const QString &folderPath, int requestedPageIndex) {
    saveCurrentHistory();

    auto book = std::make_unique<FolderBook>(folderPath);
    currentBook_ = std::move(book);

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    if (const auto *entry = currentHistoryEntry(); entry != nullptr) {
        loadingBook_ = true;
        overlayContainer_->viewer()->setViewerState({entry->lastPageIndex, entry->viewMode, entry->readingDirection});
        loadingBook_ = false;
        requestedPageIndex = entry->lastPageIndex;
    }
    displayBook(requestedPageIndex);
}

void MainWindow::openImageFile(const QString &filePath) {
    saveCurrentHistory();

    const QFileInfo fileInfo(filePath);
    auto book = std::make_unique<FolderBook>(fileInfo.dir().absolutePath());
    currentBook_ = std::move(book);

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    displayBook(pageIndexForPath(fileInfo.absoluteFilePath()));
}

void MainWindow::openArchiveBook(const QString &archivePath) {
    saveCurrentHistory();

    auto book = std::make_unique<ZipBook>(archivePath);
    currentBook_ = std::move(book);

    const QFileInfo archiveInfo(archivePath);
    overlayContainer_->sidebar()->setCurrentFolder(archiveInfo.dir().absolutePath());
    overlayContainer_->sidebar()->setCurrentArchivePath(archiveInfo.absoluteFilePath());

    int restoredPageIndex = 0;
    if (const auto *entry = currentHistoryEntry(); entry != nullptr) {
        loadingBook_ = true;
        overlayContainer_->viewer()->setViewerState({entry->lastPageIndex, entry->viewMode, entry->readingDirection});
        loadingBook_ = false;
        restoredPageIndex = entry->lastPageIndex;
    }
    displayBook(restoredPageIndex);
}

void MainWindow::displayBook(int currentPageIndex) {
    auto *viewer = overlayContainer_->viewer();
    auto *header = overlayContainer_->headerBar();

    if (!currentBook_ || currentBook_->pageCount() == 0) {
        viewer->clearPageImages();
        header->setBookPath({});
        return;
    }

    loadingBook_ = true;
    imageCache_.clear();
    viewer->clearPageImages();
    viewer->setPageCount(currentBook_->pageCount());
    viewer->setCurrentPageIndex(currentPageIndex);

    header->setBookPath(currentBook_->sourcePath());
    refreshCachedImages();

    loadingBook_ = false;
    saveCurrentHistory();
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

void MainWindow::handleCurrentPageChanged() {
    refreshCachedImages();
    saveCurrentHistory();
}

void MainWindow::refreshCachedImages() {
    if (!currentBook_) {
        return;
    }

    auto *viewer = overlayContainer_->viewer();
    const auto currentPageIndex = viewer->currentPageIndex();
    const auto pageCount = currentBook_->pageCount();
    if (pageCount <= 0) {
        viewer->retainPageImages({});
        return;
    }

    const auto startIndex = std::max(0, currentPageIndex - 2);
    const auto endIndex = std::min(pageCount - 1, currentPageIndex + 4);
    QSet<int> retainedPageIndices;

    for (int index = startIndex; index <= endIndex; ++index) {
        loadPageIntoCache(index);
        viewer->setPageImage(index, imageCache_.image(index));
        retainedPageIndices.insert(index);
    }

    if (viewer->viewMode() == ViewMode::Spread && currentPageIndex + 1 < pageCount) {
        loadPageIntoCache(currentPageIndex + 1);
        viewer->setPageImage(currentPageIndex + 1, imageCache_.image(currentPageIndex + 1));
        retainedPageIndices.insert(currentPageIndex + 1);
    }

    viewer->retainPageImages(retainedPageIndices);
}

void MainWindow::loadPageIntoCache(int pageIndex) {
    if (!currentBook_ || pageIndex < 0 || pageIndex >= currentBook_->pageCount() || imageCache_.contains(pageIndex)) {
        return;
    }

    imageCache_.insert(pageIndex, currentBook_->loadPage(pageIndex));
}

void MainWindow::loadHistory() {
    historyEntries_ = historyStore_.load();
    refreshSidebarHistory();
}

void MainWindow::saveCurrentHistory() {
    if (loadingBook_ || !currentBook_) {
        return;
    }

    auto *viewer = overlayContainer_->viewer();
    auto *entry = currentHistoryEntry();
    if (entry == nullptr) {
        historyEntries_.append({
            currentBook_->sourcePath(),
            currentBook_->type(),
            currentBook_->displayName(),
            viewer->currentPageIndex(),
            currentBook_->pageCount(),
            viewer->viewMode(),
            viewer->readingDirection(),
            QDateTime::currentDateTimeUtc(),
        });
    } else {
        entry->bookPath = currentBook_->sourcePath();
        entry->bookType = currentBook_->type();
        entry->displayName = currentBook_->displayName();
        entry->lastPageIndex = viewer->currentPageIndex();
        entry->pageCount = currentBook_->pageCount();
        entry->viewMode = viewer->viewMode();
        entry->readingDirection = viewer->readingDirection();
        entry->lastOpenedAt = QDateTime::currentDateTimeUtc();
    }

    saveHistory();
    refreshSidebarHistory();
}

void MainWindow::saveHistory() { [[maybe_unused]] const auto saved = historyStore_.save(historyEntries_); }

void MainWindow::refreshSidebarHistory() { overlayContainer_->sidebar()->setHistoryEntries(historyEntries_); }

HistoryEntry *MainWindow::currentHistoryEntry() {
    if (!currentBook_) {
        return nullptr;
    }
    return historyEntryForPath(currentBook_->sourcePath());
}

HistoryEntry *MainWindow::historyEntryForPath(const QString &bookPath) {
    const auto normalizedPath = QFileInfo(bookPath).absoluteFilePath();
    for (auto &entry : historyEntries_) {
        if (QFileInfo(entry.bookPath).absoluteFilePath() == normalizedPath) {
            return &entry;
        }
    }
    return nullptr;
}

const HistoryEntry *MainWindow::historyEntryForPath(const QString &bookPath) const {
    const auto normalizedPath = QFileInfo(bookPath).absoluteFilePath();
    for (const auto &entry : historyEntries_) {
        if (QFileInfo(entry.bookPath).absoluteFilePath() == normalizedPath) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace weeview
