#pragma once

#include "app/HistoryStore.h"
#include "model/Book.h"

#include <QMainWindow>
#include <QVector>
#include <memory>

class QCloseEvent;

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
    void displayBook(int currentPageIndex);
    [[nodiscard]] int pageIndexForPath(const QString &filePath) const;
    void loadHistory();
    void saveCurrentHistory();
    void saveHistory();
    void refreshSidebarHistory();
    [[nodiscard]] HistoryEntry *currentHistoryEntry();
    [[nodiscard]] HistoryEntry *historyEntryForPath(const QString &bookPath);
    [[nodiscard]] const HistoryEntry *historyEntryForPath(const QString &bookPath) const;

    OverlayContainer *overlayContainer_ = nullptr;
    HistoryStore historyStore_;
    QVector<HistoryEntry> historyEntries_;
    std::unique_ptr<Book> currentBook_;
    bool loadingBook_ = false;
};

} // namespace weeview
