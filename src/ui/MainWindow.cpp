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
#include <utility>

namespace weeview {

namespace {

constexpr qint64 bytesPerMiB = 1024LL * 1024LL;
constexpr qint64 decodedBytesPerPixelEstimate = 4;

qint64 cacheBytesFromMiB(int mib) { return std::max<qint64>(1, mib) * bytesPerMiB; }

qint64 estimatedDecodedBytes(const PageInfo &pageInfo) {
    if (pageInfo.imageSize.isEmpty()) {
        return 0;
    }

    return static_cast<qint64>(pageInfo.imageSize.width()) * static_cast<qint64>(pageInfo.imageSize.height()) *
           decodedBytesPerPixelEstimate;
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("WeeView"));

    overlayContainer_ = new OverlayContainer(this);
    setCentralWidget(overlayContainer_);

    if (!ImageDecoder::supportsAvif()) {
        qWarning() << "Qt image plugins do not report AVIF support. AVIF files may not open in this environment.";
    }

    appSettings_ = AppSettingsStore().load();
    imageCache_.setMaxBytes(cacheBytesFromMiB(appSettings_.imageCacheMemoryLimitMiB));
    restoreWindowSettings();
    deferredPageLoadTimer_ = new QTimer(this);
    deferredPageLoadTimer_->setSingleShot(true);
    connect(deferredPageLoadTimer_, &QTimer::timeout, this, &MainWindow::executeDeferredPageLoad);

    overlayContainer_->sidebar()->setHomeFolder(appSettings_.homeFolder);
    overlayContainer_->sidebar()->setCurrentFolder(appSettings_.homeFolder);
    overlayContainer_->sidebar()->setSidebarWidth(appSettings_.sidebarWidth);

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
    appSettings_.sidebarWidth = overlayContainer_->sidebar()->sidebarWidth();
    [[maybe_unused]] const auto saved = AppSettingsStore().save(appSettings_);
}

void MainWindow::wireSidebar() {
    auto *sidebar = overlayContainer_->sidebar();
    connect(sidebar, &Sidebar::folderBookRequested, this,
            [this](const QString &folderPath) { openFolderBook(folderPath); });
    connect(sidebar, &Sidebar::imageFileRequested, this, &MainWindow::openImageFile);
    connect(sidebar, &Sidebar::archiveBookRequested, this, &MainWindow::openArchiveBook);
    connect(sidebar, &Sidebar::sidebarWidthChanged, this, [this](int width) { appSettings_.sidebarWidth = width; });

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
    const auto pageCount = currentBook_->pageCount();
    if (pageCount <= 0) {
        viewer->retainPageImages({});
        return;
    }

    QSet<int> retainedPageIndices;
    QSet<int> protectedPageIndices;

    const auto viewerState = viewer->viewerState();
    const auto firstDisplayPageIndex = std::clamp(viewerState.currentPageIndex, 0, pageCount - 1);
    const auto lastDisplayPageIndex = viewer->viewMode() == ViewMode::Spread
                                          ? std::clamp(viewerState.currentDisplayLastPageIndex, 0, pageCount - 1)
                                          : firstDisplayPageIndex;
    const auto displayEndPageIndex = std::max(firstDisplayPageIndex, lastDisplayPageIndex);

    for (int index = firstDisplayPageIndex; index <= displayEndPageIndex; ++index) {
        loadPageIntoCache(index);
        viewer->setPageImage(index, imageCache_.image(index));
        retainedPageIndices.insert(index);
        protectedPageIndices.insert(index);
    }

    imageCache_.trimToMemoryLimit(protectedPageIndices);

    auto protectedBytes = qint64{0};
    for (const auto pageIndex : protectedPageIndices) {
        protectedBytes += imageCache_.imageSizeInBytes(pageIndex);
    }

    const auto preloadBudget = std::max<qint64>(0, imageCache_.maxBytes() - protectedBytes);
    auto nextBudget = preloadBudget * 2 / 3;
    auto previousBudget = preloadBudget - nextBudget;

    auto tryRetainPreloadPage = [&](int pageIndex, qint64 budget) -> qint64 {
        if (pageIndex < 0 || pageIndex >= pageCount || retainedPageIndices.contains(pageIndex) || budget <= 0) {
            return 0;
        }

        if (!imageCache_.contains(pageIndex)) {
            const auto estimatedBytes = estimatedDecodedBytes(currentBook_->pageInfo(pageIndex));
            if (estimatedBytes > budget) {
                return 0;
            }
            loadPageIntoCache(pageIndex);
        }

        const auto imageBytes = imageCache_.imageSizeInBytes(pageIndex);
        if (imageBytes <= 0 || imageBytes > budget) {
            return 0;
        }

        viewer->setPageImage(pageIndex, imageCache_.image(pageIndex));
        retainedPageIndices.insert(pageIndex);
        return imageBytes;
    };

    auto preloadDirection = [&](int startIndex, int step, qint64 budget) {
        auto index = startIndex;
        auto usedBytes = qint64{0};

        while (index >= 0 && index < pageCount && usedBytes < budget) {
            const auto usedForPage = tryRetainPreloadPage(index, budget - usedBytes);
            if (usedForPage <= 0 && !retainedPageIndices.contains(index)) {
                break;
            }

            usedBytes += usedForPage;
            index += step;
        }

        return std::pair{index, usedBytes};
    };

    auto [nextIndex, usedNextBytes] = preloadDirection(displayEndPageIndex + 1, 1, nextBudget);
    auto [previousIndex, usedPreviousBytes] = preloadDirection(firstDisplayPageIndex - 1, -1, previousBudget);

    nextBudget -= usedNextBytes;
    previousBudget -= usedPreviousBytes;

    if (previousBudget > 0) {
        auto [nextCarryIndex, usedCarryBytes] = preloadDirection(nextIndex, 1, previousBudget);
        nextIndex = nextCarryIndex;
        previousBudget -= usedCarryBytes;
    }

    if (nextBudget > 0) {
        auto [previousCarryIndex, usedCarryBytes] = preloadDirection(previousIndex, -1, nextBudget);
        previousIndex = previousCarryIndex;
        nextBudget -= usedCarryBytes;
    }

    imageCache_.retain(retainedPageIndices);
    imageCache_.trimToMemoryLimit(protectedPageIndices);
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
