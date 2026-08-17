#pragma once

#include "app/AppSettings.h"
#include "app/HistoryStore.h"
#include "image/ImageCache.h"
#include "model/Book.h"

#include <QMainWindow>
#include <QSet>
#include <QVector>
#include <memory>

class QCloseEvent;
class QEvent;
class QTimer;

namespace weeview {

class OverlayContainer;

class MainWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    void showRestored();
    bool openPath(const QString &path);

  protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private:
    void wireWindowControls();
    void updateHeaderWindowState();
    void restoreWindowSettings();
    void saveAppSettings();
    void applyAppSettings(const AppSettings &settings);
    void wireSidebar();
    void openFolderBook(const QString &folderPath, int requestedPageIndex = 0, bool restoreHistory = true);
    void openImageFile(const QString &filePath);
    void openArchiveBook(const QString &archivePath);
    void openAdjacentBook(int offset);
    void reloadOpenFolderBook(const QString &folderPath);
    void displayBook(const ViewerState &viewerState);
    [[nodiscard]] int pageIndexForPath(const QString &filePath) const;
    [[nodiscard]] SidebarSortSettings sortSettingsForFolder(const QString &folderPath) const;
    void handleCurrentPageChanged();
    void scheduleDeferredPageLoad();
    void executeDeferredPageLoad();
    void cancelDeferredPageLoad();
    void refreshCachedImages();
    void cancelImagePreload();
    void cancelImageLoads();
    void processImagePreloadBatch();
    void resetPageMetadata();
    void loadPageMetadataForState(const ViewerState &viewerState);
    void requestPageMetadataLoad(int pageIndex);
    void schedulePageMetadataLoad();
    void cancelPageMetadataLoad();
    void processPageMetadataBatch();
    void requestPageImageLoad(int pageIndex);
    void loadHistory();
    void saveCurrentHistory();
    void saveHistory();
    void refreshSidebarHistory();
    void deleteHistoryEntry(const QString &bookPath);
    void deleteCurrentFolderHistory(const QString &folderPath);
    [[nodiscard]] bool historyEntryBelongsToFolder(const HistoryEntry &entry, const QString &folderPath) const;
    [[nodiscard]] HistoryEntry *currentHistoryEntry();
    [[nodiscard]] HistoryEntry *historyEntryForPath(const QString &bookPath);
    [[nodiscard]] const HistoryEntry *historyEntryForPath(const QString &bookPath) const;

    OverlayContainer *overlayContainer_ = nullptr;
    AppSettings appSettings_;
    QTimer *deferredPageLoadTimer_ = nullptr;
    QTimer *imagePreloadTimer_ = nullptr;
    QTimer *pageMetadataTimer_ = nullptr;
    HistoryStore historyStore_;
    ImageCache imageCache_;
    QVector<HistoryEntry> historyEntries_;
    QVector<int> imagePreloadQueue_;
    QVector<int> pageMetadataQueue_;
    QVector<PageInfo> pageMetadata_;
    QVector<bool> pageLandscapeFlags_;
    QSet<int> retainedImagePageIndices_;
    QSet<int> protectedImagePageIndices_;
    QSet<int> loadedPageMetadataIndices_;
    QSet<int> pendingImageLoadIndices_;
    QSet<int> pendingPageMetadataIndices_;
    std::unique_ptr<Book> currentBook_;
    qint64 imagePreloadBudgetBytes_ = 0;
    int imageLoadGeneration_ = 0;
    int pageMetadataGeneration_ = 0;
    bool loadingBook_ = false;
    bool deferredPageLoadPending_ = false;
    QString suppressedHistoryBookPath_;
};

} // namespace weeview
