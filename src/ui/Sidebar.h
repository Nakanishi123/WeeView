#pragma once

#include "model/CoreTypes.h"

#include <QVector>
#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
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
    void reload();

    [[nodiscard]] QString currentFolder() const;
    [[nodiscard]] QString homeFolder() const;
    [[nodiscard]] QString currentArchivePath() const;

  signals:
    void folderBookRequested(const QString &folderPath);
    void imageFileRequested(const QString &filePath);
    void archiveBookRequested(const QString &archivePath);

  private:
    void navigateToFolder(const QString &folderPath, bool recordHistory = true);
    void navigateHome();
    void navigateBack();
    void navigateForward();
    void navigateUp();
    void populateFileList();
    void updateNavigationButtons();
    void handleItemClicked(QListWidgetItem *item);
    void handleItemDoubleClicked(QListWidgetItem *item);
    void openPendingDirectoryClick();
    void clearPendingDirectoryClick();
    void addEntry(EntryType entryType, const QString &name, const QString &path);
    [[nodiscard]] QString displayPrefix(EntryType entryType, const QString &path) const;
    [[nodiscard]] QString readingStateText(EntryType entryType, const QString &path) const;
    [[nodiscard]] const HistoryEntry *historyEntryForPath(const QString &bookPath) const;

    QLabel *pathLabel_ = nullptr;
    QPushButton *homeButton_ = nullptr;
    QPushButton *backButton_ = nullptr;
    QPushButton *forwardButton_ = nullptr;
    QPushButton *upButton_ = nullptr;
    QPushButton *reloadButton_ = nullptr;
    QListWidget *fileList_ = nullptr;
    QTimer *pendingDirectoryClickTimer_ = nullptr;

    QString homeFolder_;
    QString currentFolder_;
    QString currentArchivePath_;
    QString pendingDirectoryClickPath_;
    QStringList backStack_;
    QStringList forwardStack_;
    QVector<HistoryEntry> historyEntries_;
};

} // namespace weeview
