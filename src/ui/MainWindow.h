#pragma once

#include "app/AppSettings.h"
#include "app/HistoryStore.h"
#include "image/ImageCache.h"
#include "model/Book.h"

#include <QMainWindow>
#include <QVector>
#include <memory>

class QCloseEvent;
class QTimer;

namespace weeview {

class OverlayContainer;

class MainWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    void wireSidebar();
    void openFolderBook(const QString &folderPath, int requestedPageIndex = 0);
    void openImageFile(const QString &filePath);
    void openArchiveBook(const QString &archivePath);
    void displayBook(const ViewerState &viewerState);
    [[nodiscard]] int pageIndexForPath(const QString &filePath) const;
    void handleCurrentPageChanged();
    void scheduleDeferredPageLoad();
    void executeDeferredPageLoad();
    void cancelDeferredPageLoad();
    void refreshCachedImages();
    void loadPageIntoCache(int pageIndex);
    void loadHistory();
    void saveCurrentHistory();
    void saveHistory();
    void refreshSidebarHistory();
    [[nodiscard]] HistoryEntry *currentHistoryEntry();
    [[nodiscard]] HistoryEntry *historyEntryForPath(const QString &bookPath);
    [[nodiscard]] const HistoryEntry *historyEntryForPath(const QString &bookPath) const;

    OverlayContainer *overlayContainer_ = nullptr;
    AppSettings appSettings_;
    QTimer *deferredPageLoadTimer_ = nullptr;
    HistoryStore historyStore_;
    ImageCache imageCache_;
    QVector<HistoryEntry> historyEntries_;
    std::unique_ptr<Book> currentBook_;
    bool loadingBook_ = false;
    bool deferredPageLoadPending_ = false;
};

} // namespace weeview
