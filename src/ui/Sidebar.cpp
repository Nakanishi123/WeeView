#include "Sidebar.h"

#include "IconUtils.h"
#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPushButton>
#include <QSize>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace weeview {
namespace {

constexpr int entryTypeRole = Qt::UserRole;
constexpr int entryPathRole = Qt::UserRole + 1;
constexpr int folderSingleClickDelayMs = 160;
constexpr int navigationButtonSize = 37;
constexpr int navigationIconSize = 23;
constexpr int resizeHandleWidth = 8;
constexpr int minimumSidebarWidth = 220;
constexpr int maximumSidebarWidth = 720;
const QColor iconColor(245, 245, 245);

QString normalizedFolderPath(const QString &folderPath) {
    const QFileInfo info(folderPath);
    return info.isDir() ? info.absoluteFilePath() : QDir::homePath();
}

QPushButton *createNavigationButton(const QString &iconPath, const QString &label, QWidget *parent) {
    auto *button = new QPushButton(parent);
    button->setIcon(icons::tintedSvgIcon(iconPath, iconColor, navigationIconSize, 0));
    button->setIconSize(QSize(navigationIconSize, navigationIconSize));
    button->setToolTip(label);
    button->setAccessibleName(label);
    button->setFixedSize(navigationButtonSize, navigationButtonSize);
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

} // namespace

Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setMouseTracking(true);
    setFixedWidth(320);
    setStyleSheet(QStringLiteral("Sidebar { background: rgba(28, 28, 28, 235); color: white; }"
                                 "QLabel { color: white; }"
                                 "QListWidget { background: rgba(18, 18, 18, 245); color: white; }"
                                 "QPushButton { padding: 4px 8px; }"));

    pathLabel_ = new QLabel(this);
    pathLabel_->setTextFormat(Qt::PlainText);
    pathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel_->setWordWrap(true);

    homeButton_ = createNavigationButton(QStringLiteral(":/assets/home.svg"), tr("Home"), this);
    backButton_ = createNavigationButton(QStringLiteral(":/assets/wrap_back.svg"), tr("Back"), this);
    forwardButton_ = createNavigationButton(QStringLiteral(":/assets/wrap_forward.svg"), tr("Forward"), this);
    upButton_ = createNavigationButton(QStringLiteral(":/assets/arrow_up.svg"), tr("Up"), this);
    reloadButton_ = createNavigationButton(QStringLiteral(":/assets/undo_history.svg"), tr("Reload"), this);
    fileList_ = new QListWidget(this);
    pendingDirectoryClickTimer_ = new QTimer(this);
    pendingDirectoryClickTimer_->setSingleShot(true);

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
    connect(fileList_, &QListWidget::itemClicked, this, &Sidebar::handleItemClicked);
    connect(fileList_, &QListWidget::itemDoubleClicked, this, &Sidebar::handleItemDoubleClicked);
    connect(pendingDirectoryClickTimer_, &QTimer::timeout, this, &Sidebar::openPendingDirectoryClick);

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

void Sidebar::setHistoryEntries(const QVector<HistoryEntry> &historyEntries) {
    historyEntries_ = historyEntries;
    populateFileList();
}

void Sidebar::setSidebarWidth(int width) {
    const auto clampedWidth = std::clamp(width, minimumSidebarWidth, maximumSidebarWidth);
    if (this->width() == clampedWidth) {
        return;
    }

    setFixedWidth(clampedWidth);
    emit sidebarWidthChanged(clampedWidth);
}

void Sidebar::reload() { populateFileList(); }

QString Sidebar::currentFolder() const { return currentFolder_; }

QString Sidebar::homeFolder() const { return homeFolder_; }

QString Sidebar::currentArchivePath() const { return currentArchivePath_; }

int Sidebar::sidebarWidth() const { return width(); }

void Sidebar::mouseMoveEvent(QMouseEvent *event) {
    if (resizing_) {
        const auto delta = event->globalPosition().toPoint().x() - resizeStartGlobalX_;
        setSidebarWidth(resizeStartWidth_ + delta);
        event->accept();
        return;
    }

    if (isResizeHandlePosition(event->position().toPoint())) {
        setCursor(Qt::SizeHorCursor);
    } else {
        unsetCursor();
    }

    QWidget::mouseMoveEvent(event);
}

void Sidebar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && isResizeHandlePosition(event->position().toPoint())) {
        resizing_ = true;
        resizeStartGlobalX_ = event->globalPosition().toPoint().x();
        resizeStartWidth_ = width();
        setCursor(Qt::SizeHorCursor);
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void Sidebar::mouseReleaseEvent(QMouseEvent *event) {
    if (resizing_ && event->button() == Qt::LeftButton) {
        resizing_ = false;
        if (!isResizeHandlePosition(event->position().toPoint())) {
            unsetCursor();
        }
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void Sidebar::navigateToFolder(const QString &folderPath, bool recordHistory) {
    clearPendingDirectoryClick();

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

void Sidebar::handleItemClicked(QListWidgetItem *item) {
    if (item == nullptr) {
        return;
    }

    const auto entryType = static_cast<EntryType>(item->data(entryTypeRole).toInt());
    const auto entryPath = item->data(entryPathRole).toString();

    switch (entryType) {
    case EntryType::Directory:
        pendingDirectoryClickPath_ = entryPath;
        pendingDirectoryClickTimer_->start(std::min(folderSingleClickDelayMs, QApplication::doubleClickInterval()));
        break;
    case EntryType::Image:
        clearPendingDirectoryClick();
        emit imageFileRequested(entryPath);
        break;
    case EntryType::Archive:
        clearPendingDirectoryClick();
        emit archiveBookRequested(entryPath);
        break;
    }
}

void Sidebar::handleItemDoubleClicked(QListWidgetItem *item) {
    if (item == nullptr) {
        return;
    }

    const auto entryType = static_cast<EntryType>(item->data(entryTypeRole).toInt());
    if (entryType != EntryType::Directory) {
        return;
    }

    clearPendingDirectoryClick();
    navigateToFolder(item->data(entryPathRole).toString());
}

void Sidebar::openPendingDirectoryClick() {
    const auto folderPath = pendingDirectoryClickPath_;
    pendingDirectoryClickPath_.clear();
    if (folderPath.isEmpty()) {
        return;
    }

    emit folderBookRequested(folderPath);
}

void Sidebar::clearPendingDirectoryClick() {
    if (pendingDirectoryClickTimer_ != nullptr) {
        pendingDirectoryClickTimer_->stop();
    }
    pendingDirectoryClickPath_.clear();
}

void Sidebar::addEntry(EntryType entryType, const QString &name, const QString &path) {
    auto *item = new QListWidgetItem(displayPrefix(entryType, path) + name, fileList_);
    item->setData(entryTypeRole, static_cast<int>(entryType));
    item->setData(entryPathRole, QFileInfo(path).absoluteFilePath());

    if (entryType == EntryType::Archive && QFileInfo(path).absoluteFilePath() == currentArchivePath_) {
        item->setSelected(true);
    }
}

QString Sidebar::displayPrefix(EntryType entryType, const QString &path) const {
    switch (entryType) {
    case EntryType::Directory:
        return QStringLiteral("[D] ");
    case EntryType::Image:
        return QStringLiteral("[I][%1] ").arg(readingStateText(entryType, path));
    case EntryType::Archive:
        return QStringLiteral("[Z][%1] ").arg(readingStateText(entryType, path));
    }
    return {};
}

QString Sidebar::readingStateText(EntryType entryType, const QString &path) const {
    QString bookPath;
    if (entryType == EntryType::Image) {
        bookPath = QFileInfo(path).dir().absolutePath();
    } else if (entryType == EntryType::Archive) {
        bookPath = QFileInfo(path).absoluteFilePath();
    } else {
        return QStringLiteral("U");
    }

    const auto *entry = historyEntryForPath(bookPath);
    if (entry == nullptr) {
        return QStringLiteral("U");
    }
    if (entry->pageCount <= 0) {
        return QStringLiteral("U");
    }
    if (entry->lastPageIndex >= entry->pageCount - 1) {
        return QStringLiteral("C");
    }
    if (entry->lastPageIndex > 0) {
        return QStringLiteral("R");
    }
    return QStringLiteral("U");
}

const HistoryEntry *Sidebar::historyEntryForPath(const QString &bookPath) const {
    const auto normalizedPath = QFileInfo(bookPath).absoluteFilePath();
    for (const auto &entry : historyEntries_) {
        if (QFileInfo(entry.bookPath).absoluteFilePath() == normalizedPath) {
            return &entry;
        }
    }
    return nullptr;
}

bool Sidebar::isResizeHandlePosition(const QPoint &position) const {
    return position.x() >= width() - resizeHandleWidth;
}

} // namespace weeview
