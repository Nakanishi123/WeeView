#pragma once

#include <QWidget>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

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
    void handleItemActivated(QListWidgetItem *item);
    void addEntry(EntryType entryType, const QString &name, const QString &path);

    QLabel *pathLabel_ = nullptr;
    QPushButton *homeButton_ = nullptr;
    QPushButton *backButton_ = nullptr;
    QPushButton *forwardButton_ = nullptr;
    QPushButton *upButton_ = nullptr;
    QPushButton *reloadButton_ = nullptr;
    QListWidget *fileList_ = nullptr;

    QString homeFolder_;
    QString currentFolder_;
    QString currentArchivePath_;
    QStringList backStack_;
    QStringList forwardStack_;
};

} // namespace weeview
