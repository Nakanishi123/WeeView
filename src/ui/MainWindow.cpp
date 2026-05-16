#include "MainWindow.h"

#include "app/AppSettingsStore.h"
#include "image/ImageDecoder.h"
#include "model/ArchiveBook.h"
#include "model/FolderBook.h"
#include "ui/HeaderBar.h"
#include "ui/MangaView.h"
#include "ui/OverlayContainer.h"
#include "ui/Sidebar.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <QWindow>
#include <QtConcurrent>

#include <algorithm>
#include <utility>

namespace weeview {

namespace {

constexpr qint64 bytesPerMiB = 1024LL * 1024LL;
constexpr qint64 decodedBytesPerPixelEstimate = 4;
constexpr int metadataPagesPerBatch = 1;
constexpr int maxConcurrentImageLoads = 2;
constexpr int maxConcurrentPageMetadataLoads = 2;
constexpr int preloadPagesPerBatch = 2;
constexpr qsizetype maxHistoryEntries = 200;

struct ImageLoadResult {
    int generation = 0;
    BookType bookType = BookType::Folder;
    QString sourcePath;
    int pageIndex = -1;
    QImage image;
};

struct PageMetadataLoadResult {
    int generation = 0;
    BookType bookType = BookType::Folder;
    QString sourcePath;
    int pageIndex = -1;
    PageInfo pageInfo;
};

qint64 cacheBytesFromMiB(int mib) { return std::max<qint64>(1, mib) * bytesPerMiB; }

qint64 estimatedDecodedBytes(const PageInfo &pageInfo) {
    if (pageInfo.imageSize.isEmpty()) {
        return 0;
    }

    return static_cast<qint64>(pageInfo.imageSize.width()) * static_cast<qint64>(pageInfo.imageSize.height()) *
           decodedBytesPerPixelEstimate;
}

ImageLoadResult loadPageImageInBackground(int generation, BookType bookType, const QString &sourcePath, int pageIndex) {
    std::unique_ptr<Book> book;
    if (bookType == BookType::Archive) {
        book = std::make_unique<ArchiveBook>(sourcePath);
    } else {
        book = std::make_unique<FolderBook>(sourcePath);
    }

    return {
        generation, bookType, sourcePath, pageIndex, book ? book->loadPage(pageIndex) : QImage(),
    };
}

PageMetadataLoadResult loadPageMetadataInBackground(int generation, BookType bookType, const QString &sourcePath,
                                                    int pageIndex) {
    std::unique_ptr<Book> book;
    if (bookType == BookType::Archive) {
        book = std::make_unique<ArchiveBook>(sourcePath);
    } else {
        book = std::make_unique<FolderBook>(sourcePath);
    }

    return {
        generation, bookType, sourcePath, pageIndex, book ? book->loadPageInfo(pageIndex) : PageInfo(),
    };
}

void trimHistoryEntries(QVector<HistoryEntry> &entries) {
    std::stable_sort(entries.begin(), entries.end(), [](const HistoryEntry &left, const HistoryEntry &right) {
        return left.lastOpenedAt > right.lastOpenedAt;
    });

    QSet<QString> seenBookPaths;
    qsizetype writeIndex = 0;
    for (const auto &entry : std::as_const(entries)) {
        if (entry.bookPath.isEmpty()) {
            continue;
        }

        const auto normalizedPath = QFileInfo(entry.bookPath).absoluteFilePath();
        if (normalizedPath.isEmpty() || seenBookPaths.contains(normalizedPath)) {
            continue;
        }

        seenBookPaths.insert(normalizedPath);
        entries[writeIndex] = entry;
        ++writeIndex;
        if (writeIndex >= maxHistoryEntries) {
            break;
        }
    }

    entries.resize(writeIndex);
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("WeeView"));
    setWindowFlag(Qt::FramelessWindowHint, true);

    overlayContainer_ = new OverlayContainer(this);
    setCentralWidget(overlayContainer_);

    if (!ImageDecoder::supportsAvif()) {
        qWarning() << "Qt image plugins do not report AVIF support. AVIF files may not open in this environment.";
    }

    appSettings_ = AppSettingsStore().load();
    applyAppSettings(appSettings_);
    restoreWindowSettings();
    deferredPageLoadTimer_ = new QTimer(this);
    deferredPageLoadTimer_->setSingleShot(true);
    connect(deferredPageLoadTimer_, &QTimer::timeout, this, &MainWindow::executeDeferredPageLoad);
    imagePreloadTimer_ = new QTimer(this);
    imagePreloadTimer_->setSingleShot(true);
    connect(imagePreloadTimer_, &QTimer::timeout, this, &MainWindow::processImagePreloadBatch);
    pageMetadataTimer_ = new QTimer(this);
    pageMetadataTimer_->setSingleShot(true);
    connect(pageMetadataTimer_, &QTimer::timeout, this, &MainWindow::processPageMetadataBatch);

    overlayContainer_->sidebar()->setHomeFolder(appSettings_.homeFolder);
    overlayContainer_->sidebar()->setCurrentFolder(appSettings_.homeFolder);
    overlayContainer_->sidebar()->setSidebarWidth(appSettings_.sidebarWidth);
    overlayContainer_->sidebar()->setAppSettings(appSettings_);

    loadHistory();
    wireSidebar();
    wireWindowControls();
    updateHeaderWindowState();
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        updateHeaderWindowState();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    cancelDeferredPageLoad();
    cancelImagePreload();
    cancelImageLoads();
    cancelPageMetadataLoad();
    saveCurrentHistory();
    saveAppSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::wireWindowControls() {
    auto *header = overlayContainer_->headerBar();
    connect(header, &HeaderBar::minimizeRequested, this, &MainWindow::showMinimized);
    connect(header, &HeaderBar::maximizeRestoreRequested, this, [this] {
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
        updateHeaderWindowState();
    });
    connect(header, &HeaderBar::closeRequested, this, &MainWindow::close);
    connect(overlayContainer_, &OverlayContainer::windowResizeRequested, this, [this](Qt::Edges edges) {
        if (!isMaximized()) {
            if (auto *handle = windowHandle(); handle != nullptr) {
                handle->startSystemResize(edges);
            }
        }
    });
}

void MainWindow::updateHeaderWindowState() { overlayContainer_->headerBar()->setWindowMaximized(isMaximized()); }

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

void MainWindow::applyAppSettings(const AppSettings &settings) {
    appSettings_ = settings;
    imageCache_.setMaxBytes(cacheBytesFromMiB(appSettings_.imageCacheMemoryLimitMiB));
    overlayContainer_->setOverlaySettings(appSettings_.overlayEdgeTriggerSize, appSettings_.overlayHideDelayMs);
}

void MainWindow::wireSidebar() {
    auto *sidebar = overlayContainer_->sidebar();
    connect(sidebar, &Sidebar::folderBookRequested, this,
            [this](const QString &folderPath) { openFolderBook(folderPath); });
    connect(sidebar, &Sidebar::imageFileRequested, this, &MainWindow::openImageFile);
    connect(sidebar, &Sidebar::archiveBookRequested, this, &MainWindow::openArchiveBook);
    connect(sidebar, &Sidebar::sidebarWidthChanged, this, [this](int width) { appSettings_.sidebarWidth = width; });
    connect(sidebar, &Sidebar::historyEntryDeleteRequested, this, &MainWindow::deleteHistoryEntry);
    connect(sidebar, &Sidebar::currentFolderHistoryDeleteRequested, this, &MainWindow::deleteCurrentFolderHistory);
    connect(sidebar, &Sidebar::appSettingsChanged, this, [this, sidebar](const AppSettings &settings) {
        applyAppSettings(settings);
        sidebar->setHomeFolder(appSettings_.homeFolder);
        sidebar->setSidebarWidth(appSettings_.sidebarWidth);
        sidebar->setAppSettings(appSettings_);
        saveAppSettings();
    });

    auto *viewer = overlayContainer_->viewer();
    connect(viewer, &MangaView::currentPageIndexChanged, this, &MainWindow::handleCurrentPageChanged);
    connect(viewer, &MangaView::viewModeChanged, this, [this] {
        suppressedHistoryBookPath_.clear();
        cancelDeferredPageLoad();
        refreshCachedImages();
        saveCurrentHistory();
    });
    connect(viewer, &MangaView::readingDirectionChanged, this, [this] {
        suppressedHistoryBookPath_.clear();
        saveCurrentHistory();
    });
}

void MainWindow::openFolderBook(const QString &folderPath, int requestedPageIndex) {
    cancelDeferredPageLoad();
    cancelImagePreload();
    cancelImageLoads();
    cancelPageMetadataLoad();
    saveCurrentHistory();

    auto book = std::make_unique<FolderBook>(folderPath);
    currentBook_ = std::move(book);
    suppressedHistoryBookPath_.clear();

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    ViewerState viewerState;
    viewerState.currentPageIndex = requestedPageIndex;
    viewerState.currentDisplayLastPageIndex = requestedPageIndex;
    viewerState.viewMode = appSettings_.defaultViewMode;
    viewerState.readingDirection = appSettings_.defaultReadingDirection;
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
    cancelImagePreload();
    cancelImageLoads();
    cancelPageMetadataLoad();
    saveCurrentHistory();

    const QFileInfo fileInfo(filePath);
    auto book = std::make_unique<FolderBook>(fileInfo.dir().absolutePath());
    currentBook_ = std::move(book);
    suppressedHistoryBookPath_.clear();

    overlayContainer_->sidebar()->setCurrentArchivePath({});
    const auto requestedPageIndex = pageIndexForPath(fileInfo.absoluteFilePath());
    displayBook({
        requestedPageIndex,
        requestedPageIndex,
        appSettings_.defaultViewMode,
        appSettings_.defaultReadingDirection,
        SpreadGroupDirection::Forward,
    });
}

void MainWindow::openArchiveBook(const QString &archivePath) {
    cancelDeferredPageLoad();
    cancelImagePreload();
    cancelImageLoads();
    cancelPageMetadataLoad();
    saveCurrentHistory();

    auto book = std::make_unique<ArchiveBook>(archivePath);
    currentBook_ = std::move(book);
    suppressedHistoryBookPath_.clear();

    const QFileInfo archiveInfo(archivePath);
    overlayContainer_->sidebar()->setCurrentFolder(archiveInfo.dir().absolutePath());
    overlayContainer_->sidebar()->setCurrentArchivePath(archiveInfo.absoluteFilePath());

    ViewerState viewerState;
    viewerState.viewMode = appSettings_.defaultViewMode;
    viewerState.readingDirection = appSettings_.defaultReadingDirection;
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
        resetPageMetadata();
        viewer->clearPageImages();
        header->setBookPath({});
        return;
    }

    loadingBook_ = true;
    imageCache_.clear();
    viewer->clearPageImages();
    viewer->setPageCount(currentBook_->pageCount());
    resetPageMetadata();
    loadPageMetadataForState(viewerState);
    viewer->setPageLandscapeFlags(pageLandscapeFlags_);
    viewer->setViewerState(viewerState);

    header->setBookPath(currentBook_->sourcePath());
    refreshCachedImages();
    schedulePageMetadataLoad();

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

    suppressedHistoryBookPath_.clear();
    cancelImagePreload();
    loadPageMetadataForState(overlayContainer_->viewer()->viewerState());
    overlayContainer_->viewer()->setPageLandscapeFlags(pageLandscapeFlags_);
    schedulePageMetadataLoad();
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
    cancelImagePreload();

    if (!currentBook_) {
        return;
    }

    auto *viewer = overlayContainer_->viewer();
    const auto pageCount = currentBook_->pageCount();
    if (pageCount <= 0) {
        retainedImagePageIndices_.clear();
        protectedImagePageIndices_.clear();
        viewer->retainPageImages({});
        return;
    }

    retainedImagePageIndices_.clear();
    protectedImagePageIndices_.clear();

    const auto viewerState = viewer->viewerState();
    const auto firstDisplayPageIndex = std::clamp(viewerState.currentPageIndex, 0, pageCount - 1);
    const auto lastDisplayPageIndex = viewer->viewMode() == ViewMode::Spread
                                          ? std::clamp(viewerState.currentDisplayLastPageIndex, 0, pageCount - 1)
                                          : firstDisplayPageIndex;
    const auto displayEndPageIndex = std::max(firstDisplayPageIndex, lastDisplayPageIndex);

    for (int index = firstDisplayPageIndex; index <= displayEndPageIndex; ++index) {
        retainedImagePageIndices_.insert(index);
        protectedImagePageIndices_.insert(index);
        if (imageCache_.contains(index)) {
            viewer->setPageImage(index, imageCache_.image(index));
        } else {
            requestPageImageLoad(index);
        }
    }

    imageCache_.trimToMemoryLimit(protectedImagePageIndices_);

    auto protectedBytes = qint64{0};
    for (const auto pageIndex : protectedImagePageIndices_) {
        protectedBytes += imageCache_.imageSizeInBytes(pageIndex);
    }

    const auto preloadBudget = std::max<qint64>(0, imageCache_.maxBytes() - protectedBytes);
    auto nextBudget = preloadBudget * 2 / 3;
    auto previousBudget = preloadBudget - nextBudget;
    auto retainedPreloadBytes = qint64{0};

    imagePreloadBudgetBytes_ = preloadBudget;

    auto tryQueuePreloadPage = [&](int pageIndex, qint64 budget) -> qint64 {
        if (pageIndex < 0 || pageIndex >= pageCount || retainedImagePageIndices_.contains(pageIndex) || budget <= 0) {
            return 0;
        }

        const auto imageBytes = imageCache_.contains(pageIndex) ? imageCache_.imageSizeInBytes(pageIndex)
                                                                : estimatedDecodedBytes(pageMetadata_.value(pageIndex));
        if (imageBytes <= 0 || imageBytes > budget) {
            return 0;
        }

        imagePreloadQueue_.append(pageIndex);
        if (imageCache_.contains(pageIndex)) {
            viewer->setPageImage(pageIndex, imageCache_.image(pageIndex));
            retainedImagePageIndices_.insert(pageIndex);
            retainedPreloadBytes += imageBytes;
        }

        return imageBytes;
    };

    auto preloadDirection = [&](int startIndex, int step, qint64 budget) {
        auto index = startIndex;
        auto usedBytes = qint64{0};

        while (index >= 0 && index < pageCount && usedBytes < budget) {
            const auto usedForPage = tryQueuePreloadPage(index, budget - usedBytes);
            if (usedForPage <= 0 && !retainedImagePageIndices_.contains(index)) {
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

    imagePreloadBudgetBytes_ = std::max<qint64>(0, imagePreloadBudgetBytes_ - retainedPreloadBytes);
    imageCache_.retain(retainedImagePageIndices_);
    imageCache_.trimToMemoryLimit(protectedImagePageIndices_);
    viewer->retainPageImages(retainedImagePageIndices_);

    if (!imagePreloadQueue_.isEmpty() && imagePreloadTimer_ != nullptr) {
        imagePreloadTimer_->start(0);
    }
}

void MainWindow::cancelImagePreload() {
    if (imagePreloadTimer_ != nullptr) {
        imagePreloadTimer_->stop();
    }
    imagePreloadQueue_.clear();
    imagePreloadBudgetBytes_ = 0;
}

void MainWindow::cancelImageLoads() {
    ++imageLoadGeneration_;
    pendingImageLoadIndices_.clear();
}

void MainWindow::processImagePreloadBatch() {
    if (!currentBook_ || imagePreloadQueue_.isEmpty() || imagePreloadBudgetBytes_ <= 0) {
        imagePreloadQueue_.clear();
        return;
    }

    auto *viewer = overlayContainer_->viewer();
    auto processedCount = 0;
    while (!imagePreloadQueue_.isEmpty() && processedCount < preloadPagesPerBatch && imagePreloadBudgetBytes_ > 0 &&
           pendingImageLoadIndices_.size() < maxConcurrentImageLoads) {
        const auto pageIndex = imagePreloadQueue_.takeFirst();
        if (pageIndex < 0 || pageIndex >= currentBook_->pageCount() || retainedImagePageIndices_.contains(pageIndex)) {
            continue;
        }

        ++processedCount;
        if (imageCache_.contains(pageIndex)) {
            const auto imageBytes = imageCache_.imageSizeInBytes(pageIndex);
            if (imageBytes <= 0 || imageBytes > imagePreloadBudgetBytes_) {
                imageCache_.retain(retainedImagePageIndices_);
                imageCache_.trimToMemoryLimit(protectedImagePageIndices_);
                viewer->retainPageImages(retainedImagePageIndices_);
                continue;
            }

            imagePreloadBudgetBytes_ -= imageBytes;
            viewer->setPageImage(pageIndex, imageCache_.image(pageIndex));
            retainedImagePageIndices_.insert(pageIndex);
            continue;
        }

        const auto estimatedBytes = estimatedDecodedBytes(pageMetadata_.value(pageIndex));
        if (estimatedBytes <= 0 || estimatedBytes > imagePreloadBudgetBytes_) {
            continue;
        }

        imagePreloadBudgetBytes_ -= estimatedBytes;
        retainedImagePageIndices_.insert(pageIndex);
        requestPageImageLoad(pageIndex);
    }

    imageCache_.retain(retainedImagePageIndices_);
    imageCache_.trimToMemoryLimit(protectedImagePageIndices_);
    viewer->retainPageImages(retainedImagePageIndices_);

    if (!imagePreloadQueue_.isEmpty() && imagePreloadBudgetBytes_ > 0 && imagePreloadTimer_ != nullptr &&
        pendingImageLoadIndices_.size() < maxConcurrentImageLoads) {
        imagePreloadTimer_->start(0);
    }
}

void MainWindow::resetPageMetadata() {
    pageMetadataQueue_.clear();
    pageMetadata_.clear();
    pageLandscapeFlags_.clear();
    loadedPageMetadataIndices_.clear();
    pendingPageMetadataIndices_.clear();

    if (!currentBook_ || currentBook_->pageCount() <= 0) {
        return;
    }

    const auto pageCount = currentBook_->pageCount();
    pageMetadata_.reserve(pageCount);
    pageLandscapeFlags_.resize(pageCount);
    for (int index = 0; index < pageCount; ++index) {
        pageMetadata_.append(currentBook_->pageInfo(index));
    }
}

void MainWindow::loadPageMetadataForState(const ViewerState &viewerState) {
    if (!currentBook_) {
        return;
    }

    const auto pageCount = currentBook_->pageCount();
    if (pageCount <= 0) {
        return;
    }

    auto loadIfInRange = [this, pageCount](int pageIndex) {
        if (pageIndex >= 0 && pageIndex < pageCount) {
            requestPageMetadataLoad(pageIndex);
        }
    };

    loadIfInRange(viewerState.currentPageIndex);
    loadIfInRange(viewerState.currentDisplayLastPageIndex);

    if (viewerState.viewMode == ViewMode::Spread) {
        loadIfInRange(viewerState.currentPageIndex - 1);
        loadIfInRange(viewerState.currentPageIndex + 1);
        loadIfInRange(viewerState.currentDisplayLastPageIndex - 1);
        loadIfInRange(viewerState.currentDisplayLastPageIndex + 1);
    }
}

void MainWindow::requestPageMetadataLoad(int pageIndex) {
    if (!currentBook_ || pageIndex < 0 || pageIndex >= currentBook_->pageCount() ||
        loadedPageMetadataIndices_.contains(pageIndex) || pendingPageMetadataIndices_.contains(pageIndex)) {
        return;
    }

    pendingPageMetadataIndices_.insert(pageIndex);

    auto *watcher = new QFutureWatcher<PageMetadataLoadResult>(this);
    const auto generation = pageMetadataGeneration_;
    const auto bookType = currentBook_->type();
    const auto sourcePath = currentBook_->sourcePath();
    connect(watcher, &QFutureWatcher<PageMetadataLoadResult>::finished, this, [this, watcher] {
        const auto result = watcher->result();
        watcher->deleteLater();

        if (result.generation != pageMetadataGeneration_) {
            return;
        }

        pendingPageMetadataIndices_.remove(result.pageIndex);

        if (!currentBook_ || result.pageIndex < 0 || result.pageIndex >= currentBook_->pageCount() ||
            result.bookType != currentBook_->type() ||
            QFileInfo(result.sourcePath).absoluteFilePath() !=
                QFileInfo(currentBook_->sourcePath()).absoluteFilePath() ||
            result.pageInfo.displayPath.isEmpty()) {
            return;
        }

        if (pageMetadata_.size() < currentBook_->pageCount()) {
            resetPageMetadata();
        }
        pageMetadata_[result.pageIndex] = result.pageInfo;
        pageLandscapeFlags_[result.pageIndex] = result.pageInfo.isLandscape;
        loadedPageMetadataIndices_.insert(result.pageIndex);

        overlayContainer_->viewer()->setPageLandscapeFlags(pageLandscapeFlags_);
        if (imagePreloadQueue_.isEmpty()) {
            refreshCachedImages();
        }

        if (!pageMetadataQueue_.isEmpty() && pageMetadataTimer_ != nullptr &&
            pendingPageMetadataIndices_.size() < maxConcurrentPageMetadataLoads) {
            pageMetadataTimer_->start(0);
        }
    });
    watcher->setFuture(QtConcurrent::run(loadPageMetadataInBackground, generation, bookType, sourcePath, pageIndex));
}

void MainWindow::schedulePageMetadataLoad() {
    if (!currentBook_ || currentBook_->pageCount() <= 0) {
        return;
    }

    const auto pageCount = currentBook_->pageCount();
    const auto currentPageIndex = std::clamp(overlayContainer_->viewer()->currentPageIndex(), 0, pageCount - 1);
    pageMetadataQueue_.clear();

    for (int index = currentPageIndex + 1; index < pageCount; ++index) {
        if (!loadedPageMetadataIndices_.contains(index) && !pendingPageMetadataIndices_.contains(index)) {
            pageMetadataQueue_.append(index);
        }
    }
    for (int index = currentPageIndex - 1; index >= 0; --index) {
        if (!loadedPageMetadataIndices_.contains(index) && !pendingPageMetadataIndices_.contains(index)) {
            pageMetadataQueue_.append(index);
        }
    }
    if (!loadedPageMetadataIndices_.contains(currentPageIndex) &&
        !pendingPageMetadataIndices_.contains(currentPageIndex)) {
        pageMetadataQueue_.prepend(currentPageIndex);
    }

    if (!pageMetadataQueue_.isEmpty() && pageMetadataTimer_ != nullptr) {
        pageMetadataTimer_->start(0);
    }
}

void MainWindow::cancelPageMetadataLoad() {
    if (pageMetadataTimer_ != nullptr) {
        pageMetadataTimer_->stop();
    }
    pageMetadataQueue_.clear();
    pendingPageMetadataIndices_.clear();
    ++pageMetadataGeneration_;
}

void MainWindow::processPageMetadataBatch() {
    if (!currentBook_ || pageMetadataQueue_.isEmpty()) {
        pageMetadataQueue_.clear();
        return;
    }

    auto processedCount = 0;
    while (!pageMetadataQueue_.isEmpty() && processedCount < metadataPagesPerBatch &&
           pendingPageMetadataIndices_.size() < maxConcurrentPageMetadataLoads) {
        const auto pageIndex = pageMetadataQueue_.takeFirst();
        if (pageIndex < 0 || pageIndex >= currentBook_->pageCount() || loadedPageMetadataIndices_.contains(pageIndex) ||
            pendingPageMetadataIndices_.contains(pageIndex)) {
            continue;
        }

        ++processedCount;
        requestPageMetadataLoad(pageIndex);
    }

    if (!pageMetadataQueue_.isEmpty() && pageMetadataTimer_ != nullptr &&
        pendingPageMetadataIndices_.size() < maxConcurrentPageMetadataLoads) {
        pageMetadataTimer_->start(0);
    }
}

void MainWindow::requestPageImageLoad(int pageIndex) {
    if (!currentBook_ || pageIndex < 0 || pageIndex >= currentBook_->pageCount() || imageCache_.contains(pageIndex) ||
        pendingImageLoadIndices_.contains(pageIndex)) {
        return;
    }

    pendingImageLoadIndices_.insert(pageIndex);

    auto *watcher = new QFutureWatcher<ImageLoadResult>(this);
    const auto generation = imageLoadGeneration_;
    const auto bookType = currentBook_->type();
    const auto sourcePath = currentBook_->sourcePath();
    connect(watcher, &QFutureWatcher<ImageLoadResult>::finished, this, [this, watcher] {
        const auto result = watcher->result();
        watcher->deleteLater();

        if (result.generation != imageLoadGeneration_) {
            return;
        }

        pendingImageLoadIndices_.remove(result.pageIndex);

        if (!currentBook_ || result.pageIndex < 0 || result.pageIndex >= currentBook_->pageCount() ||
            result.bookType != currentBook_->type() ||
            QFileInfo(result.sourcePath).absoluteFilePath() !=
                QFileInfo(currentBook_->sourcePath()).absoluteFilePath() ||
            result.image.isNull()) {
            return;
        }

        imageCache_.insert(result.pageIndex, result.image);
        imageCache_.trimToMemoryLimit(protectedImagePageIndices_);

        auto *viewer = overlayContainer_->viewer();
        if (retainedImagePageIndices_.contains(result.pageIndex) && imageCache_.contains(result.pageIndex)) {
            viewer->setPageImage(result.pageIndex, imageCache_.image(result.pageIndex));
        }
        viewer->retainPageImages(retainedImagePageIndices_);

        if (!imagePreloadQueue_.isEmpty() && imagePreloadBudgetBytes_ > 0 && imagePreloadTimer_ != nullptr &&
            pendingImageLoadIndices_.size() < maxConcurrentImageLoads) {
            imagePreloadTimer_->start(0);
        }
    });
    watcher->setFuture(QtConcurrent::run(loadPageImageInBackground, generation, bookType, sourcePath, pageIndex));
}

void MainWindow::loadHistory() {
    historyEntries_ = historyStore_.load();
    trimHistoryEntries(historyEntries_);
    refreshSidebarHistory();
}

void MainWindow::saveCurrentHistory() {
    if (loadingBook_ || !currentBook_) {
        return;
    }
    if (!suppressedHistoryBookPath_.isEmpty() &&
        QFileInfo(currentBook_->sourcePath()).absoluteFilePath() == suppressedHistoryBookPath_) {
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

    trimHistoryEntries(historyEntries_);
    saveHistory();
    refreshSidebarHistory();
}

void MainWindow::saveHistory() { [[maybe_unused]] const auto saved = historyStore_.save(historyEntries_); }

void MainWindow::refreshSidebarHistory() { overlayContainer_->sidebar()->setHistoryEntries(historyEntries_); }

void MainWindow::deleteHistoryEntry(const QString &bookPath) {
    if (bookPath.isEmpty()) {
        return;
    }

    saveCurrentHistory();

    const auto normalizedPath = QFileInfo(bookPath).absoluteFilePath();
    const auto oldSize = historyEntries_.size();
    historyEntries_.erase(std::remove_if(historyEntries_.begin(), historyEntries_.end(),
                                         [&normalizedPath](const HistoryEntry &entry) {
                                             return QFileInfo(entry.bookPath).absoluteFilePath() == normalizedPath;
                                         }),
                          historyEntries_.end());
    if (historyEntries_.size() == oldSize) {
        return;
    }

    if (currentBook_ != nullptr && QFileInfo(currentBook_->sourcePath()).absoluteFilePath() == normalizedPath) {
        suppressedHistoryBookPath_ = normalizedPath;
    }

    saveHistory();
    refreshSidebarHistory();
}

void MainWindow::deleteCurrentFolderHistory(const QString &folderPath) {
    if (folderPath.isEmpty()) {
        return;
    }

    saveCurrentHistory();

    const auto normalizedFolderPath = QFileInfo(folderPath).absoluteFilePath();
    const auto oldSize = historyEntries_.size();
    historyEntries_.erase(std::remove_if(historyEntries_.begin(), historyEntries_.end(),
                                         [this, &normalizedFolderPath](const HistoryEntry &entry) {
                                             return historyEntryBelongsToFolder(entry, normalizedFolderPath);
                                         }),
                          historyEntries_.end());
    if (historyEntries_.size() == oldSize) {
        return;
    }

    if (currentBook_ != nullptr) {
        const auto currentBookPath = QFileInfo(currentBook_->sourcePath()).absoluteFilePath();
        HistoryEntry currentEntry;
        currentEntry.bookPath = currentBook_->sourcePath();
        currentEntry.bookType = currentBook_->type();
        if (historyEntryBelongsToFolder(currentEntry, normalizedFolderPath)) {
            suppressedHistoryBookPath_ = currentBookPath;
        }
    }

    saveHistory();
    refreshSidebarHistory();
}

bool MainWindow::historyEntryBelongsToFolder(const HistoryEntry &entry, const QString &folderPath) const {
    const auto normalizedFolderPath = QFileInfo(folderPath).absoluteFilePath();
    const auto bookInfo = QFileInfo(entry.bookPath);
    const auto bookPath = bookInfo.absoluteFilePath();

    if (entry.bookType == BookType::Archive) {
        return bookInfo.dir().absolutePath() == normalizedFolderPath;
    }

    return bookPath == normalizedFolderPath || bookInfo.dir().absolutePath() == normalizedFolderPath;
}

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
