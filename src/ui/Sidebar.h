#pragma once

#include "app/AppSettings.h"
#include "model/CoreTypes.h"

#include <QVector>
#include <QWidget>

class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMouseEvent;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTimer;

namespace weeview {

class Sidebar final : public QWidget {
    Q_OBJECT

  public:
    enum class EntryType {
        Directory,
        Image,
        Archive,
    };

    explicit Sidebar(QWidget *parent = nullptr);

    void setHomeFolder(const QString &homeFolder);
    void setCurrentFolder(const QString &folderPath);
    void setCurrentArchivePath(const QString &archivePath);
    void setHistoryEntries(const QVector<HistoryEntry> &historyEntries);
    void setAppSettings(const AppSettings &settings);
    void setSidebarWidth(int width);

    [[nodiscard]] QString currentFolder() const;
    [[nodiscard]] QString homeFolder() const;
    [[nodiscard]] QString currentArchivePath() const;
    [[nodiscard]] int sidebarWidth() const;

  signals:
    void folderBookRequested(const QString &folderPath);
    void imageFileRequested(const QString &filePath);
    void archiveBookRequested(const QString &archivePath);
    void appSettingsChanged(const AppSettings &settings);
    void sidebarWidthChanged(int width);
    void historyEntryDeleteRequested(const QString &bookPath);
    void currentFolderHistoryDeleteRequested(const QString &folderPath);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    void navigateToFolder(const QString &folderPath, bool recordHistory = true);
    void navigateHome();
    void navigateBack();
    void navigateForward();
    void navigateUp();
    void showHistory();
    void showSettings();
    void showSortMenu();
    void populateFileList();
    void populateHistoryList();
    void populateSettingsPanel();
    void emitSettingsChanged();
    void browseHomeFolder();
    void updateNavigationButtons();
    void handleItemClicked(QListWidgetItem *item);
    void handleItemDoubleClicked(QListWidgetItem *item);
    void showFileListContextMenu(const QPoint &position);
    void openPendingDirectoryClick();
    void clearPendingDirectoryClick();
    void addEntry(EntryType entryType, const QString &name, const QString &path);
    void addHistoryEntry(const HistoryEntry &entry);
    void loadHistoryThumbnailAsync(const HistoryEntry &entry, int requestId);
    void updateFileListReadingStates();
    void updateArchiveSelection();
    void applySortSettingsForCurrentFolder();
    void saveSortSettingsForCurrentFolder();
    void updateSortButtonText();
    [[nodiscard]] QString historyBookPathForItem(const QListWidgetItem *item) const;
    [[nodiscard]] int readingState(EntryType entryType, const QString &path) const;
    [[nodiscard]] const HistoryEntry *historyEntryForPath(const QString &bookPath) const;
    void updateResizeCursor(const QPoint &position);
    [[nodiscard]] bool isResizeHandlePosition(const QPoint &position) const;

    QLabel *pathLabel_ = nullptr;
    QPushButton *homeButton_ = nullptr;
    QPushButton *backButton_ = nullptr;
    QPushButton *forwardButton_ = nullptr;
    QPushButton *upButton_ = nullptr;
    QPushButton *historyButton_ = nullptr;
    QPushButton *settingsButton_ = nullptr;
    QPushButton *sortButton_ = nullptr;
    QStackedWidget *contentStack_ = nullptr;
    QListWidget *fileList_ = nullptr;
    QWidget *settingsPanel_ = nullptr;
    QLineEdit *homeFolderEdit_ = nullptr;
    QComboBox *defaultReadingDirectionCombo_ = nullptr;
    QComboBox *defaultViewModeCombo_ = nullptr;
    QSpinBox *overlayEdgeTriggerSizeSpin_ = nullptr;
    QSpinBox *overlayHideDelaySpin_ = nullptr;
    QSpinBox *pageLoadDebounceSpin_ = nullptr;
    QSpinBox *imageCacheMemoryLimitSpin_ = nullptr;
    QSpinBox *sidebarWidthSpin_ = nullptr;
    QTimer *pendingDirectoryClickTimer_ = nullptr;

    AppSettings appSettings_;
    QString homeFolder_;
    QString currentFolder_;
    QString currentArchivePath_;
    QString pendingDirectoryClickPath_;
    QStringList backStack_;
    QStringList forwardStack_;
    QVector<HistoryEntry> historyEntries_;
    SidebarSortKey sortKey_ = SidebarSortKey::FileName;
    SidebarSortOrder sortOrder_ = SidebarSortOrder::Ascending;
    bool showingHistory_ = false;
    bool showingSettings_ = false;
    bool updatingSettingsControls_ = false;
    int historyThumbnailGeneration_ = 0;
    bool resizing_ = false;
    int resizeStartGlobalX_ = 0;
    int resizeStartWidth_ = 0;
};

} // namespace weeview
