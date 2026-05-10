#include "Sidebar.h"

#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

namespace weeview {
namespace {

constexpr int entryTypeRole = Qt::UserRole;
constexpr int entryPathRole = Qt::UserRole + 1;

QString normalizedFolderPath(const QString &folderPath) {
    const QFileInfo info(folderPath);
    return info.isDir() ? info.absoluteFilePath() : QDir::homePath();
}

QString displayPrefix(Sidebar::EntryType entryType) {
    switch (entryType) {
    case Sidebar::EntryType::Directory:
        return QStringLiteral("[D] ");
    case Sidebar::EntryType::Image:
        return QStringLiteral("[I][U] ");
    case Sidebar::EntryType::Archive:
        return QStringLiteral("[Z][U] ");
    }
    return {};
}

} // namespace

Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setFixedWidth(320);
    setStyleSheet(QStringLiteral("Sidebar { background: rgba(28, 28, 28, 235); color: white; }"
                                 "QLabel { color: white; }"
                                 "QListWidget { background: rgba(18, 18, 18, 245); color: white; }"
                                 "QPushButton { padding: 4px 8px; }"));

    pathLabel_ = new QLabel(this);
    pathLabel_->setTextFormat(Qt::PlainText);
    pathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel_->setWordWrap(true);

    homeButton_ = new QPushButton(QStringLiteral("Home"), this);
    backButton_ = new QPushButton(QStringLiteral("Back"), this);
    forwardButton_ = new QPushButton(QStringLiteral("Forward"), this);
    upButton_ = new QPushButton(QStringLiteral("Up"), this);
    reloadButton_ = new QPushButton(QStringLiteral("Reload"), this);
    fileList_ = new QListWidget(this);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(4);
    buttonLayout->addWidget(homeButton_);
    buttonLayout->addWidget(backButton_);
    buttonLayout->addWidget(forwardButton_);
    buttonLayout->addWidget(upButton_);
    buttonLayout->addWidget(reloadButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(pathLabel_);
    layout->addLayout(buttonLayout);
    layout->addWidget(fileList_, 1);

    connect(homeButton_, &QPushButton::clicked, this, &Sidebar::navigateHome);
    connect(backButton_, &QPushButton::clicked, this, &Sidebar::navigateBack);
    connect(forwardButton_, &QPushButton::clicked, this, &Sidebar::navigateForward);
    connect(upButton_, &QPushButton::clicked, this, &Sidebar::navigateUp);
    connect(reloadButton_, &QPushButton::clicked, this, &Sidebar::reload);
    connect(fileList_, &QListWidget::itemActivated, this, &Sidebar::handleItemActivated);
    connect(fileList_, &QListWidget::itemClicked, this, &Sidebar::handleItemActivated);

    setHomeFolder(QDir::homePath());
}

void Sidebar::setHomeFolder(const QString &homeFolder) {
    homeFolder_ = normalizedFolderPath(homeFolder);
    if (currentFolder_.isEmpty()) {
        navigateToFolder(homeFolder_, false);
    }
}

void Sidebar::setCurrentFolder(const QString &folderPath) { navigateToFolder(folderPath, false); }

void Sidebar::setCurrentArchivePath(const QString &archivePath) {
    currentArchivePath_ = archivePath.isEmpty() ? QString() : QFileInfo(archivePath).absoluteFilePath();
    populateFileList();
}

void Sidebar::reload() { populateFileList(); }

QString Sidebar::currentFolder() const { return currentFolder_; }

QString Sidebar::homeFolder() const { return homeFolder_; }

QString Sidebar::currentArchivePath() const { return currentArchivePath_; }

void Sidebar::navigateToFolder(const QString &folderPath, bool recordHistory) {
    const auto normalized = normalizedFolderPath(folderPath);
    if (currentFolder_ == normalized) {
        populateFileList();
        return;
    }

    if (recordHistory && !currentFolder_.isEmpty()) {
        backStack_.append(currentFolder_);
        forwardStack_.clear();
    }

    currentFolder_ = normalized;
    pathLabel_->setText(currentFolder_);
    populateFileList();
    updateNavigationButtons();
}

void Sidebar::navigateHome() { navigateToFolder(homeFolder_); }

void Sidebar::navigateBack() {
    if (backStack_.isEmpty()) {
        return;
    }

    forwardStack_.append(currentFolder_);
    currentFolder_ = backStack_.takeLast();
    pathLabel_->setText(currentFolder_);
    populateFileList();
    updateNavigationButtons();
}

void Sidebar::navigateForward() {
    if (forwardStack_.isEmpty()) {
        return;
    }

    backStack_.append(currentFolder_);
    currentFolder_ = forwardStack_.takeLast();
    pathLabel_->setText(currentFolder_);
    populateFileList();
    updateNavigationButtons();
}

void Sidebar::navigateUp() {
    const QDir directory(currentFolder_);
    const auto parentPath = QFileInfo(directory.absolutePath()).dir().absolutePath();
    navigateToFolder(parentPath);
}

void Sidebar::populateFileList() {
    fileList_->clear();

    const QDir directory(currentFolder_);
    const auto entries =
        directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::NoSort);

    QStringList directoryNames;
    QStringList fileNames;
    for (const auto &entry : entries) {
        if (entry.isDir()) {
            directoryNames.append(entry.fileName());
        } else if (filetypes::isSupportedImageFile(entry.fileName()) ||
                   filetypes::isSupportedArchiveFile(entry.fileName())) {
            fileNames.append(entry.fileName());
        }
    }

    naturalsort::sort(directoryNames);
    naturalsort::sort(fileNames);

    for (const auto &name : directoryNames) {
        addEntry(EntryType::Directory, name, directory.absoluteFilePath(name));
    }

    for (const auto &name : fileNames) {
        const auto path = directory.absoluteFilePath(name);
        addEntry(filetypes::isSupportedArchiveFile(name) ? EntryType::Archive : EntryType::Image, name, path);
    }

    updateNavigationButtons();
}

void Sidebar::updateNavigationButtons() {
    backButton_->setEnabled(!backStack_.isEmpty());
    forwardButton_->setEnabled(!forwardStack_.isEmpty());
    upButton_->setEnabled(QFileInfo(currentFolder_).dir().absolutePath() != currentFolder_);
}

void Sidebar::handleItemActivated(QListWidgetItem *item) {
    if (item == nullptr) {
        return;
    }

    const auto entryType = static_cast<EntryType>(item->data(entryTypeRole).toInt());
    const auto entryPath = item->data(entryPathRole).toString();

    switch (entryType) {
    case EntryType::Directory:
        navigateToFolder(entryPath);
        break;
    case EntryType::Image:
        emit imageFileRequested(entryPath);
        break;
    case EntryType::Archive:
        emit archiveBookRequested(entryPath);
        break;
    }
}

void Sidebar::addEntry(EntryType entryType, const QString &name, const QString &path) {
    auto *item = new QListWidgetItem(displayPrefix(entryType) + name, fileList_);
    item->setData(entryTypeRole, static_cast<int>(entryType));
    item->setData(entryPathRole, QFileInfo(path).absoluteFilePath());

    if (entryType == EntryType::Archive && QFileInfo(path).absoluteFilePath() == currentArchivePath_) {
        item->setSelected(true);
    }
}

} // namespace weeview
