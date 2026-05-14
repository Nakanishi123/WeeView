#include "MainWindow.h"

#include "app/AppSettingsStore.h"
#include "image/ImageDecoder.h"
#include "model/FolderBook.h"
#include "model/ZipBook.h"
#include "ui/HeaderBar.h"
#include "ui/MangaView.h"
#include "ui/OverlayContainer.h"
#include "ui/Sidebar.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QTimer>
#include <QVector>

#include <algorithm>

namespace weeview {

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("WeeView"));

    overlayContainer_ = new OverlayContainer(this);
    setCentralWidget(overlayContainer_);

    if (!ImageDecoder::supportsAvif()) {
        qWarning() << "Qt image plugins do not report AVIF support. AVIF files may not open in this environment.";
    }

    appSettings_ = AppSettingsStore().load();
    restoreWindowSettings();
    deferredPageLoadTimer_ = new QTimer(this);
    deferredPageLoadTimer_->setSingleShot(true);
    connect(deferredPageLoadTimer_, &QTimer::timeout, this, &MainWindow::executeDeferredPageLoad);

    overlayContainer_->sidebar()->setHomeFolder(appSettings_.homeFolder);
    overlayContainer_->sidebar()->setCurrentFolder(appSettings_.homeFolder);

    loadHistory();
    wireSidebar();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    cancelDeferredPageLoad();
    saveCurrentHistory();
    saveAppSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::showRestored() {
    if (appSettings_.windowMaximized) {
        showMaximized();
        return;
    }

    show();
}

void MainWindow::restoreWindowSettings() {
    const auto width = std::max(320, appSettings_.windowWidth);
    const auto height = std::max(240, appSettings_.windowHeight);
    resize(width, height);
}

void MainWindow::saveAppSettings() {
    const auto normalSize = normalGeometry().isValid() ? normalGeometry().size() : size();
    appSettings_.windowWidth = std::max(320, normalSize.width());
    appSettings_.windowHeight = std::max(240, normalSize.height());
    appSettings_.windowMaximized = isMaximized();
    [[maybe_unused]] const auto saved = AppSettingsStore().save(appSettings_);
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
        cancelDeferredPageLoad();
        refreshCachedImages();
        saveCurrentHistory();
    });
    connect(viewer, &MangaView::readingDirectionChanged, this, [this] { saveCurrentHistory(); });
}

void MainWindow::openFolderBook(const QString &folderPath, int requestedPageIndex) {
    cancelDeferredPageLoad();
    saveCurrentHistory();

    auto book = std::make_unique<FolderBook>(folderPath);
    currentBook_ = std::move(book);

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    ViewerState viewerState;
    viewerState.currentPageIndex = requestedPageIndex;
    viewerState.currentDisplayLastPageIndex = requestedPageIndex;
    if (const auto *entry = currentHistoryEntry(); entry != nullptr) {
        viewerState = {
            entry->lastPageIndex,    entry->lastDisplayLastPageIndex, entry->viewMode,
            entry->readingDirection, entry->spreadGroupDirection,
        };
    }
    displayBook(viewerState);
}

void MainWindow::openImageFile(const QString &filePath) {
    cancelDeferredPageLoad();
    saveCurrentHistory();

    const QFileInfo fileInfo(filePath);
    auto book = std::make_unique<FolderBook>(fileInfo.dir().absolutePath());
    currentBook_ = std::move(book);

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    const auto requestedPageIndex = pageIndexForPath(fileInfo.absoluteFilePath());
    displayBook({
        requestedPageIndex,
        requestedPageIndex,
        overlayContainer_->viewer()->viewMode(),
        overlayContainer_->viewer()->readingDirection(),
        SpreadGroupDirection::Forward,
    });
}

void MainWindow::openArchiveBook(const QString &archivePath) {
    cancelDeferredPageLoad();
    saveCurrentHistory();

    auto book = std::make_unique<ZipBook>(archivePath);
    currentBook_ = std::move(book);

    const QFileInfo archiveInfo(archivePath);
    overlayContainer_->sidebar()->setCurrentFolder(archiveInfo.dir().absolutePath());
    overlayContainer_->sidebar()->setCurrentArchivePath(archiveInfo.absoluteFilePath());

    ViewerState viewerState;
    if (const auto *entry = currentHistoryEntry(); entry != nullptr) {
        viewerState = {
            entry->lastPageIndex,    entry->lastDisplayLastPageIndex, entry->viewMode,
            entry->readingDirection, entry->spreadGroupDirection,
        };
    }
    displayBook(viewerState);
}

void MainWindow::displayBook(const ViewerState &viewerState) {
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
    QVector<bool> landscapePages;
    landscapePages.reserve(currentBook_->pageCount());
    for (int index = 0; index < currentBook_->pageCount(); ++index) {
        landscapePages.append(currentBook_->pageInfo(index).isLandscape);
    }
    viewer->setPageLandscapeFlags(landscapePages);
    viewer->setViewerState(viewerState);

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
    if (loadingBook_) {
        return;
    }

    scheduleDeferredPageLoad();
}

void MainWindow::scheduleDeferredPageLoad() {
    if (appSettings_.pageLoadDebounceMs <= 0) {
        executeDeferredPageLoad();
        return;
    }

    deferredPageLoadPending_ = true;
    deferredPageLoadTimer_->start(appSettings_.pageLoadDebounceMs);
}

void MainWindow::executeDeferredPageLoad() {
    deferredPageLoadPending_ = false;
    refreshCachedImages();
    saveCurrentHistory();
}

void MainWindow::cancelDeferredPageLoad() {
    if (deferredPageLoadTimer_ != nullptr) {
        deferredPageLoadTimer_->stop();
    }
    deferredPageLoadPending_ = false;
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
    const auto viewerState = viewer->viewerState();
    auto *entry = currentHistoryEntry();
    if (entry == nullptr) {
        historyEntries_.append({
            currentBook_->sourcePath(),
            currentBook_->type(),
            currentBook_->displayName(),
            viewerState.currentPageIndex,
            viewerState.currentDisplayLastPageIndex,
            currentBook_->pageCount(),
            viewerState.viewMode,
            viewerState.readingDirection,
            viewerState.spreadGroupDirection,
            QDateTime::currentDateTimeUtc(),
        });
    } else {
        entry->bookPath = currentBook_->sourcePath();
        entry->bookType = currentBook_->type();
        entry->displayName = currentBook_->displayName();
        entry->lastPageIndex = viewerState.currentPageIndex;
        entry->lastDisplayLastPageIndex = viewerState.currentDisplayLastPageIndex;
        entry->pageCount = currentBook_->pageCount();
        entry->viewMode = viewerState.viewMode;
        entry->readingDirection = viewerState.readingDirection;
        entry->spreadGroupDirection = viewerState.spreadGroupDirection;
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
