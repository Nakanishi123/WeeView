#include "Sidebar.h"

#include "IconUtils.h"
#include "model/ArchiveBook.h"
#include "model/FolderBook.h"
#include "util/FileTypes.h"
#include "util/NaturalSort.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSize>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <algorithm>
#include <array>
#include <memory>

namespace weeview {
namespace {

constexpr int entryTypeRole = Qt::UserRole;
constexpr int entryPathRole = Qt::UserRole + 1;
constexpr int readingStateRole = Qt::UserRole + 2;
constexpr int itemKindRole = Qt::UserRole + 3;
constexpr int bookTypeRole = Qt::UserRole + 4;
constexpr int thumbnailImageRole = Qt::UserRole + 5;
constexpr int secondaryTextRole = Qt::UserRole + 6;
constexpr int progressTextRole = Qt::UserRole + 7;
constexpr int folderSingleClickDelayMs = 160;
constexpr int maxEagerHistoryThumbnails = 30;
constexpr int navigationButtonSize = 37;
constexpr int navigationIconSize = 23;
constexpr int entryIconSize = 20;
constexpr int statusIconSize = 18;
constexpr int historyThumbnailWidth = 54;
constexpr int historyThumbnailHeight = 78;
constexpr int historyThumbnailLoadWidth = 108;
constexpr int historyThumbnailLoadHeight = 156;
constexpr int historyRowHeight = 94;
constexpr int entryHorizontalPadding = 6;
constexpr int entryVerticalPadding = 3;
constexpr int entryTextGap = 8;
constexpr int statusIconReservedWidth = 24;
constexpr int fileListFontPointSizeIncrease = 1;
constexpr int resizeHandleWidth = 8;
constexpr int minimumSidebarWidth = 220;
constexpr int maximumSidebarWidth = 720;
const QColor iconColor(245, 245, 245);
const QColor folderIconColor(244, 190, 72);
const QColor statusIconColor(210, 230, 255);

enum class ReadingState {
    Unread = 0,
    Reading,
    Completed,
};

enum class SidebarItemKind {
    FileEntry = 0,
    HistoryEntry,
};

struct FileListEntry {
    Sidebar::EntryType entryType;
    QString name;
    QString path;
    QDateTime createdAt;
    QDateTime modifiedAt;
};

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

QString progressText(const HistoryEntry &entry) {
    if (entry.pageCount <= 0) {
        return {};
    }
    return QStringLiteral("%1 / %2").arg(std::min(entry.lastPageIndex + 1, entry.pageCount)).arg(entry.pageCount);
}

QDateTime sortableCreatedAt(const QFileInfo &fileInfo) {
    const auto createdAt = fileInfo.birthTime();
    if (createdAt.isValid()) {
        return createdAt;
    }
    return fileInfo.metadataChangeTime();
}

ReadingState readingStateForHistoryEntry(const HistoryEntry &entry) {
    if (entry.pageCount <= 0) {
        return ReadingState::Unread;
    }
    if (entry.lastPageIndex >= entry.pageCount - 1) {
        return ReadingState::Completed;
    }
    if (entry.lastPageIndex > 0) {
        return ReadingState::Reading;
    }
    return ReadingState::Unread;
}

QImage loadFirstPageThumbnail(HistoryEntry entry) {
    std::unique_ptr<Book> book;
    if (entry.bookType == BookType::Folder) {
        book = std::make_unique<FolderBook>(entry.bookPath);
    } else {
        book = std::make_unique<ArchiveBook>(entry.bookPath);
    }

    if (!book || book->pageCount() <= 0) {
        return {};
    }

    const auto image = book->loadPage(0);
    if (image.isNull()) {
        return {};
    }
    return image.scaled(historyThumbnailLoadWidth, historyThumbnailLoadHeight, Qt::KeepAspectRatio,
                        Qt::SmoothTransformation);
}

QIcon entryIcon(Sidebar::EntryType entryType) {
    switch (entryType) {
    case Sidebar::EntryType::Directory:
        return icons::tintedSvgIcon(QStringLiteral(":/assets/folder_closed.svg"), folderIconColor, entryIconSize, 0);
    case Sidebar::EntryType::Image:
        return icons::tintedSvgIcon(QStringLiteral(":/assets/postcard.svg"), iconColor, entryIconSize, 0);
    case Sidebar::EntryType::Archive:
        return icons::tintedSvgIcon(QStringLiteral(":/assets/book_closed.svg"), iconColor, entryIconSize, 0);
    }
    return {};
}

QIcon bookTypeIcon(BookType bookType) {
    if (bookType == BookType::Folder) {
        return icons::tintedSvgIcon(QStringLiteral(":/assets/folder_closed.svg"), folderIconColor, entryIconSize, 0);
    }
    return icons::tintedSvgIcon(QStringLiteral(":/assets/book_closed.svg"), iconColor, entryIconSize, 0);
}

QIcon readingStateIcon(ReadingState readingState) {
    switch (readingState) {
    case ReadingState::Reading:
        return icons::tintedSvgIcon(QStringLiteral(":/assets/circle.svg"), statusIconColor, statusIconSize, 0);
    case ReadingState::Completed:
        return icons::tintedSvgIcon(QStringLiteral(":/assets/check_circle_outside.svg"), statusIconColor,
                                    statusIconSize, 0);
    case ReadingState::Unread:
        return {};
    }
    return {};
}

QRect visibleItemRect(const QStyleOptionViewItem &option) {
    auto rect = option.rect;
    if (option.widget == nullptr) {
        return rect;
    }

    const auto viewportRect = option.widget->rect();
    rect.setLeft(std::max(rect.left(), viewportRect.left()));
    rect.setRight(std::min(rect.right(), viewportRect.right()));
    return rect;
}

QRect visibleViewportRect(QRect rect, const QWidget *viewport) {
    if (viewport == nullptr) {
        return rect;
    }

    const auto viewportRect = viewport->rect();
    rect.setLeft(std::max(rect.left(), viewportRect.left()));
    rect.setRight(std::min(rect.right(), viewportRect.right()));
    return rect;
}

class SidebarItemDelegate final : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const auto itemKind = static_cast<SidebarItemKind>(index.data(itemKindRole).toInt());
        if (itemKind == SidebarItemKind::HistoryEntry) {
            paintHistoryItem(painter, option, index);
            return;
        }
        paintFileItem(painter, option, index);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        auto hint = QStyledItemDelegate::sizeHint(option, index);
        const auto itemKind = static_cast<SidebarItemKind>(index.data(itemKindRole).toInt());
        if (itemKind == SidebarItemKind::HistoryEntry) {
            hint.setHeight(historyRowHeight);
            return hint;
        }
        hint.setHeight(hint.height() + 2);
        return hint;
    }

  private:
    void paintFileItem(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        QStyleOptionViewItem viewOption(option);
        initStyleOption(&viewOption, index);
        viewOption.text.clear();
        viewOption.icon = {};

        auto *style = viewOption.widget != nullptr ? viewOption.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, viewOption.widget);

        const auto contentRect = visibleItemRect(option).adjusted(entryHorizontalPadding, entryVerticalPadding,
                                                                  -entryHorizontalPadding, -entryVerticalPadding);
        const auto iconRect =
            QRect(contentRect.left(), contentRect.top() + ((contentRect.height() - entryIconSize) / 2), entryIconSize,
                  entryIconSize);
        const auto statusRect =
            QRect(contentRect.right() - statusIconSize + 1,
                  contentRect.top() + ((contentRect.height() - statusIconSize) / 2), statusIconSize, statusIconSize);
        const auto textLeft = iconRect.right() + 1 + entryTextGap;
        const auto textRight = contentRect.right() - statusIconReservedWidth;
        const auto textRect =
            QRect(textLeft, contentRect.top(), std::max(0, textRight - textLeft + 1), contentRect.height());

        const auto entryType = static_cast<Sidebar::EntryType>(index.data(entryTypeRole).toInt());
        entryIcon(entryType).paint(painter, iconRect);

        const auto selected = option.state.testFlag(QStyle::State_Selected);
        painter->save();
        painter->setPen(selected ? option.palette.highlightedText().color() : option.palette.text().color());
        const auto elidedText =
            option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textRect.width());
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elidedText);
        painter->restore();

        const auto state = static_cast<ReadingState>(index.data(readingStateRole).toInt());
        const auto statusIcon = readingStateIcon(state);
        if (!statusIcon.isNull()) {
            statusIcon.paint(painter, statusRect);
        }
    }

    void paintHistoryItem(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        QStyleOptionViewItem viewOption(option);
        initStyleOption(&viewOption, index);
        viewOption.text.clear();
        viewOption.icon = {};

        auto *style = viewOption.widget != nullptr ? viewOption.widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, viewOption.widget);

        const auto contentRect =
            visibleItemRect(option).adjusted(entryHorizontalPadding, 8, -entryHorizontalPadding, -8);
        const auto thumbnailRect =
            QRect(contentRect.left(), contentRect.top(), historyThumbnailWidth, historyThumbnailHeight);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillRect(thumbnailRect, QColor(34, 34, 34));
        painter->setPen(QColor(75, 75, 75));
        painter->drawRect(thumbnailRect.adjusted(0, 0, -1, -1));

        const auto thumbnail = index.data(thumbnailImageRole).value<QImage>();
        if (!thumbnail.isNull()) {
            const auto pixmap = QPixmap::fromImage(thumbnail);
            const auto scaled = pixmap.scaled(thumbnailRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            const auto target = QRect(thumbnailRect.left() + ((thumbnailRect.width() - scaled.width()) / 2),
                                      thumbnailRect.top() + ((thumbnailRect.height() - scaled.height()) / 2),
                                      scaled.width(), scaled.height());
            painter->drawPixmap(target, scaled);
        } else {
            const auto bookType = static_cast<BookType>(index.data(bookTypeRole).toInt());
            const auto placeholderSize = 28;
            const auto placeholderRect = QRect(thumbnailRect.left() + ((thumbnailRect.width() - placeholderSize) / 2),
                                               thumbnailRect.top() + ((thumbnailRect.height() - placeholderSize) / 2),
                                               placeholderSize, placeholderSize);
            bookTypeIcon(bookType).paint(painter, placeholderRect);
        }
        painter->restore();

        const auto statusRect =
            QRect(contentRect.right() - statusIconSize + 1,
                  contentRect.top() + ((contentRect.height() - statusIconSize) / 2), statusIconSize, statusIconSize);
        const auto textLeft = thumbnailRect.right() + 1 + entryTextGap;
        const auto textRight = contentRect.right() - statusIconReservedWidth;
        const auto textWidth = std::max(0, textRight - textLeft + 1);
        const auto selected = option.state.testFlag(QStyle::State_Selected);
        const auto primaryColor = selected ? option.palette.highlightedText().color() : option.palette.text().color();
        const auto secondaryColor = selected ? option.palette.highlightedText().color() : QColor(185, 185, 185);

        painter->save();
        painter->setPen(primaryColor);
        const auto titleRect = QRect(textLeft, contentRect.top() + 4, textWidth, 24);
        painter->drawText(
            titleRect, Qt::AlignVCenter | Qt::AlignLeft,
            option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, titleRect.width()));

        painter->setPen(secondaryColor);
        const auto pathRect = QRect(textLeft, titleRect.bottom() + 3, textWidth, 20);
        painter->drawText(
            pathRect, Qt::AlignVCenter | Qt::AlignLeft,
            option.fontMetrics.elidedText(index.data(secondaryTextRole).toString(), Qt::ElideRight, pathRect.width()));

        const auto progressRect = QRect(textLeft, pathRect.bottom() + 3, textWidth, 18);
        painter->drawText(progressRect, Qt::AlignVCenter | Qt::AlignLeft,
                          option.fontMetrics.elidedText(index.data(progressTextRole).toString(), Qt::ElideRight,
                                                        progressRect.width()));
        painter->restore();

        const auto state = static_cast<ReadingState>(index.data(readingStateRole).toInt());
        const auto statusIcon = readingStateIcon(state);
        if (!statusIcon.isNull()) {
            statusIcon.paint(painter, statusRect);
        }
    }
};

} // namespace

Sidebar::Sidebar(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setMouseTracking(true);
    setFixedWidth(320);
    setStyleSheet(QStringLiteral("Sidebar { background: rgba(28, 28, 28, 235); color: white; }"
                                 "QLabel { color: white; }"
                                 "QListWidget { background: rgba(18, 18, 18, 245); color: white; }"
                                 "QScrollArea { background: rgba(18, 18, 18, 245); border: none; }"
                                 "QWidget#settingsPanel { background: rgba(18, 18, 18, 245); color: white; }"
                                 "QLineEdit, QComboBox, QSpinBox { padding: 4px; }"
                                 "QPushButton { padding: 4px 8px; }"));

    pathLabel_ = new QLabel(this);
    pathLabel_->setTextFormat(Qt::PlainText);
    pathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel_->setWordWrap(true);

    homeButton_ = createNavigationButton(QStringLiteral(":/assets/home.svg"), tr("Home"), this);
    backButton_ = createNavigationButton(QStringLiteral(":/assets/wrap_back.svg"), tr("Back"), this);
    forwardButton_ = createNavigationButton(QStringLiteral(":/assets/wrap_forward.svg"), tr("Forward"), this);
    upButton_ = createNavigationButton(QStringLiteral(":/assets/arrow_up.svg"), tr("Up"), this);
    historyButton_ = createNavigationButton(QStringLiteral(":/assets/undo_history.svg"), tr("History"), this);
    settingsButton_ = createNavigationButton(QStringLiteral(":/assets/settings.svg"), tr("Settings"), this);
    sortButton_ = new QPushButton(this);
    sortButton_->setFocusPolicy(Qt::NoFocus);
    sortButton_->setToolTip(tr("Change file list sort order"));
    sortButton_->setAccessibleName(tr("File list sort order"));
    updateSortButtonText();

    fileList_ = new QListWidget(this);
    fileList_->setMouseTracking(true);
    fileList_->viewport()->setMouseTracking(true);
    fileList_->setItemDelegate(new SidebarItemDelegate(fileList_));
    fileList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    fileList_->setTextElideMode(Qt::ElideRight);
    fileList_->setWordWrap(false);
    fileList_->viewport()->setContextMenuPolicy(Qt::CustomContextMenu);
    auto fileListFont = fileList_->font();
    if (fileListFont.pointSize() > 0) {
        fileListFont.setPointSize(fileListFont.pointSize() + fileListFontPointSizeIncrease);
    } else if (fileListFont.pixelSize() > 0) {
        fileListFont.setPixelSize(fileListFont.pixelSize() + fileListFontPointSizeIncrease);
    }
    fileList_->setFont(fileListFont);
    contentStack_ = new QStackedWidget(this);
    contentStack_->addWidget(fileList_);

    auto *settingsScrollArea = new QScrollArea(this);
    settingsScrollArea->setWidgetResizable(true);
    settingsPanel_ = new QWidget(settingsScrollArea);
    settingsPanel_->setObjectName(QStringLiteral("settingsPanel"));
    settingsScrollArea->setWidget(settingsPanel_);
    contentStack_->addWidget(settingsScrollArea);
    populateSettingsPanel();

    const std::array<QWidget *, 14> cursorUpdateWidgets = {pathLabel_,
                                                           homeButton_,
                                                           backButton_,
                                                           forwardButton_,
                                                           upButton_,
                                                           historyButton_,
                                                           settingsButton_,
                                                           sortButton_,
                                                           contentStack_,
                                                           fileList_,
                                                           fileList_->viewport(),
                                                           settingsScrollArea,
                                                           settingsScrollArea->viewport(),
                                                           settingsPanel_};
    for (auto *widget : cursorUpdateWidgets) {
        widget->installEventFilter(this);
    }

    pendingDirectoryClickTimer_ = new QTimer(this);
    pendingDirectoryClickTimer_->setSingleShot(true);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(4);
    buttonLayout->addWidget(homeButton_);
    buttonLayout->addWidget(backButton_);
    buttonLayout->addWidget(forwardButton_);
    buttonLayout->addWidget(upButton_);
    buttonLayout->addWidget(historyButton_);
    buttonLayout->addWidget(settingsButton_);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->addWidget(pathLabel_);
    layout->addLayout(buttonLayout);
    layout->addWidget(sortButton_);
    layout->addWidget(contentStack_, 1);

    connect(homeButton_, &QPushButton::clicked, this, &Sidebar::navigateHome);
    connect(backButton_, &QPushButton::clicked, this, &Sidebar::navigateBack);
    connect(forwardButton_, &QPushButton::clicked, this, &Sidebar::navigateForward);
    connect(upButton_, &QPushButton::clicked, this, &Sidebar::navigateUp);
    connect(historyButton_, &QPushButton::clicked, this, &Sidebar::showHistory);
    connect(settingsButton_, &QPushButton::clicked, this, &Sidebar::showSettings);
    connect(sortButton_, &QPushButton::clicked, this, &Sidebar::showSortMenu);
    connect(fileList_, &QListWidget::itemClicked, this, &Sidebar::handleItemClicked);
    connect(fileList_, &QListWidget::itemDoubleClicked, this, &Sidebar::handleItemDoubleClicked);
    connect(fileList_->viewport(), &QWidget::customContextMenuRequested, this, &Sidebar::showFileListContextMenu);
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

void Sidebar::setCurrentFolderBookPath(const QString &folderPath) {
    currentFolderBookPath_ = folderPath.isEmpty() ? QString() : QFileInfo(folderPath).absoluteFilePath();
    updateCurrentBookSelection();
}

void Sidebar::setCurrentArchivePath(const QString &archivePath) {
    currentArchivePath_ = archivePath.isEmpty() ? QString() : QFileInfo(archivePath).absoluteFilePath();
    updateCurrentBookSelection();
}

void Sidebar::setHistoryEntries(const QVector<HistoryEntry> &historyEntries) {
    historyEntries_ = historyEntries;
    if (showingHistory_) {
        populateHistoryList();
    } else if (!showingSettings_) {
        updateFileListReadingStates();
    }
}

void Sidebar::setAppSettings(const AppSettings &settings) {
    appSettings_ = settings;
    updatingSettingsControls_ = true;
    const QSignalBlocker homeBlocker(homeFolderEdit_);
    const QSignalBlocker readingBlocker(defaultReadingDirectionCombo_);
    const QSignalBlocker viewModeBlocker(defaultViewModeCombo_);
    const QSignalBlocker edgeBlocker(overlayEdgeTriggerSizeSpin_);
    const QSignalBlocker hideBlocker(overlayHideDelaySpin_);
    const QSignalBlocker debounceBlocker(pageLoadDebounceSpin_);
    const QSignalBlocker cacheBlocker(imageCacheMemoryLimitSpin_);
    const QSignalBlocker widthBlocker(sidebarWidthSpin_);

    homeFolderEdit_->setText(appSettings_.homeFolder);
    defaultReadingDirectionCombo_->setCurrentIndex(
        appSettings_.defaultReadingDirection == ReadingDirection::RightToLeft ? 0 : 1);
    defaultViewModeCombo_->setCurrentIndex(appSettings_.defaultViewMode == ViewMode::SinglePage ? 0 : 1);
    overlayEdgeTriggerSizeSpin_->setValue(appSettings_.overlayEdgeTriggerSize);
    overlayHideDelaySpin_->setValue(appSettings_.overlayHideDelayMs);
    pageLoadDebounceSpin_->setValue(appSettings_.pageLoadDebounceMs);
    imageCacheMemoryLimitSpin_->setValue(appSettings_.imageCacheMemoryLimitMiB);
    sidebarWidthSpin_->setValue(appSettings_.sidebarWidth);
    updatingSettingsControls_ = false;
    applySortSettingsForCurrentFolder();
    updateSortButtonText();
    if (!showingHistory_ && !showingSettings_ && !currentFolder_.isEmpty()) {
        populateFileList();
    }
}

void Sidebar::setSidebarWidth(int width) {
    const auto clampedWidth = std::clamp(width, minimumSidebarWidth, maximumSidebarWidth);
    if (this->width() == clampedWidth) {
        return;
    }

    setFixedWidth(clampedWidth);
    if (sidebarWidthSpin_ != nullptr && sidebarWidthSpin_->value() != clampedWidth) {
        const QSignalBlocker blocker(sidebarWidthSpin_);
        sidebarWidthSpin_->setValue(clampedWidth);
    }
    emit sidebarWidthChanged(clampedWidth);
}

QString Sidebar::currentFolder() const { return currentFolder_; }

QString Sidebar::homeFolder() const { return homeFolder_; }

QString Sidebar::currentFolderBookPath() const { return currentFolderBookPath_; }

QString Sidebar::currentArchivePath() const { return currentArchivePath_; }

int Sidebar::sidebarWidth() const { return width(); }

bool Sidebar::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::ToolTip && (watched == fileList_ || watched == fileList_->viewport())) {
        auto *helpEvent = static_cast<QHelpEvent *>(event);
        if (showFileListItemToolTip(helpEvent)) {
            event->accept();
            return true;
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }

    if (event->type() == QEvent::ContextMenu && (watched == fileList_ || watched == fileList_->viewport())) {
        auto *contextMenuEvent = static_cast<QContextMenuEvent *>(event);
        const auto viewportPosition = watched == fileList_->viewport()
                                          ? contextMenuEvent->pos()
                                          : fileList_->viewport()->mapFrom(fileList_, contextMenuEvent->pos());
        showFileListContextMenu(viewportPosition);
        event->accept();
        return true;
    }

    if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove) {
        updateResizeCursor(mapFromGlobal(QCursor::pos()));
    }

    Q_UNUSED(watched);
    return QWidget::eventFilter(watched, event);
}

void Sidebar::mouseMoveEvent(QMouseEvent *event) {
    if (resizing_) {
        const auto delta = event->globalPosition().toPoint().x() - resizeStartGlobalX_;
        setSidebarWidth(resizeStartWidth_ + delta);
        event->accept();
        return;
    }

    updateResizeCursor(event->position().toPoint());
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

void Sidebar::navigateToFolder(const QString &folderPath, bool recordHistory, const QString &entryPathToReveal) {
    clearPendingDirectoryClick();
    const auto wasShowingAlternateView = showingHistory_ || showingSettings_;
    showingHistory_ = false;
    showingSettings_ = false;
    ++historyThumbnailGeneration_;

    const auto normalized = normalizedFolderPath(folderPath);
    const auto revealPath = entryPathToReveal.isEmpty() ? QString() : QFileInfo(entryPathToReveal).absoluteFilePath();
    if (currentFolder_ == normalized) {
        if (wasShowingAlternateView || contentStack_->currentIndex() != 0) {
            populateFileList();
        } else {
            contentStack_->setCurrentIndex(0);
            pathLabel_->setText(currentFolder_);
        }
        if (!revealPath.isEmpty() && fileListEntryForPath(revealPath) != nullptr) {
            selectFileListEntry(revealPath, true);
        }
        updateNavigationButtons();
        return;
    }

    if (recordHistory && !currentFolder_.isEmpty()) {
        backStack_.append(currentFolder_);
        forwardStack_.clear();
    }

    currentFolder_ = normalized;
    applySortSettingsForCurrentFolder();
    pathLabel_->setText(currentFolder_);
    populateFileList();
    if (!revealPath.isEmpty() && fileListEntryForPath(revealPath) != nullptr) {
        selectFileListEntry(revealPath, true);
    }
    updateNavigationButtons();
}

void Sidebar::navigateHome() { navigateToFolder(homeFolder_); }

void Sidebar::navigateBack() {
    if (backStack_.isEmpty()) {
        return;
    }

    const auto originFolder = currentFolder_;
    forwardStack_.append(originFolder);
    currentFolder_ = backStack_.takeLast();
    applySortSettingsForCurrentFolder();
    pathLabel_->setText(currentFolder_);
    populateFileList();
    if (fileListEntryForPath(originFolder) != nullptr) {
        selectFileListEntry(originFolder, true);
    }
    updateNavigationButtons();
}

void Sidebar::navigateForward() {
    if (forwardStack_.isEmpty()) {
        return;
    }

    const auto originFolder = currentFolder_;
    backStack_.append(originFolder);
    currentFolder_ = forwardStack_.takeLast();
    applySortSettingsForCurrentFolder();
    pathLabel_->setText(currentFolder_);
    populateFileList();
    if (fileListEntryForPath(originFolder) != nullptr) {
        selectFileListEntry(originFolder, true);
    }
    updateNavigationButtons();
}

void Sidebar::navigateUp() {
    const QDir directory(currentFolder_);
    const auto parentPath = QFileInfo(directory.absolutePath()).dir().absolutePath();
    navigateToFolder(parentPath, true, currentFolder_);
}

void Sidebar::showHistory() {
    if (showingHistory_) {
        populateFileList();
        return;
    }

    populateHistoryList();
}

void Sidebar::showSettings() {
    if (showingSettings_) {
        populateFileList();
        return;
    }

    clearPendingDirectoryClick();
    showingHistory_ = false;
    showingSettings_ = true;
    ++historyThumbnailGeneration_;
    pathLabel_->setText(tr("Settings"));
    sortButton_->hide();
    contentStack_->setCurrentIndex(1);
    updateNavigationButtons();
}

void Sidebar::showSortMenu() {
    QMenu menu(this);

    const auto addSortAction = [this, &menu](const QString &label, SidebarSortKey key, SidebarSortOrder order) {
        auto *action = menu.addAction(label);
        action->setCheckable(true);
        action->setChecked(sortKey_ == key && sortOrder_ == order);
        connect(action, &QAction::triggered, this, [this, key, order]() {
            sortKey_ = key;
            sortOrder_ = order;
            updateSortButtonText();
            saveSortSettingsForCurrentFolder();
            populateFileList();
        });
    };

    addSortAction(QStringLiteral("↑ ") + tr("Filename"), SidebarSortKey::FileName, SidebarSortOrder::Ascending);
    addSortAction(QStringLiteral("↓ ") + tr("Filename"), SidebarSortKey::FileName, SidebarSortOrder::Descending);
    menu.addSeparator();
    addSortAction(QStringLiteral("↑ ") + tr("Created"), SidebarSortKey::CreatedAt, SidebarSortOrder::Ascending);
    addSortAction(QStringLiteral("↓ ") + tr("Created"), SidebarSortKey::CreatedAt, SidebarSortOrder::Descending);
    menu.addSeparator();
    addSortAction(QStringLiteral("↑ ") + tr("Modified"), SidebarSortKey::ModifiedAt, SidebarSortOrder::Ascending);
    addSortAction(QStringLiteral("↓ ") + tr("Modified"), SidebarSortKey::ModifiedAt, SidebarSortOrder::Descending);

    menu.exec(sortButton_->mapToGlobal(QPoint(0, sortButton_->height())));
}

void Sidebar::populateFileList() {
    showingHistory_ = false;
    showingSettings_ = false;
    ++historyThumbnailGeneration_;
    sortButton_->show();
    contentStack_->setCurrentIndex(0);
    fileList_->clear();
    pathLabel_->setText(currentFolder_);

    const QDir directory(currentFolder_);
    const auto entries =
        directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::NoSort);

    QVector<FileListEntry> directoryEntries;
    QVector<FileListEntry> fileEntries;
    for (const auto &entry : entries) {
        const auto item = FileListEntry{
            entry.isDir()
                ? EntryType::Directory
                : (filetypes::isSupportedArchiveFile(entry.fileName()) ? EntryType::Archive : EntryType::Image),
            entry.fileName(),
            entry.absoluteFilePath(),
            sortableCreatedAt(entry),
            entry.lastModified(),
        };

        if (entry.isDir()) {
            directoryEntries.append(item);
        } else if (filetypes::isSupportedImageFile(entry.fileName()) ||
                   filetypes::isSupportedArchiveFile(entry.fileName())) {
            fileEntries.append(item);
        }
    }

    const auto sortEntries = [this](QVector<FileListEntry> &items) {
        std::sort(items.begin(), items.end(), [this](const FileListEntry &left, const FileListEntry &right) {
            auto result = 0;
            switch (sortKey_) {
            case SidebarSortKey::FileName:
                result = naturalsort::lessThan(left.name, right.name)
                             ? -1
                             : (naturalsort::lessThan(right.name, left.name) ? 1 : 0);
                break;
            case SidebarSortKey::CreatedAt:
                result = left.createdAt < right.createdAt ? -1 : (right.createdAt < left.createdAt ? 1 : 0);
                break;
            case SidebarSortKey::ModifiedAt:
                result = left.modifiedAt < right.modifiedAt ? -1 : (right.modifiedAt < left.modifiedAt ? 1 : 0);
                break;
            }

            if (result == 0) {
                result = naturalsort::lessThan(left.name, right.name)
                             ? -1
                             : (naturalsort::lessThan(right.name, left.name) ? 1 : 0);
            }

            return sortOrder_ == SidebarSortOrder::Ascending ? result < 0 : result > 0;
        });
    };

    sortEntries(directoryEntries);
    sortEntries(fileEntries);

    for (const auto &entry : directoryEntries) {
        addEntry(entry.entryType, entry.name, entry.path);
    }

    for (const auto &entry : fileEntries) {
        addEntry(entry.entryType, entry.name, entry.path);
    }

    updateCurrentBookSelection();
    updateNavigationButtons();
}

void Sidebar::populateHistoryList() {
    clearPendingDirectoryClick();
    showingHistory_ = true;
    showingSettings_ = false;
    const auto requestId = ++historyThumbnailGeneration_;
    sortButton_->hide();
    contentStack_->setCurrentIndex(0);
    fileList_->clear();
    pathLabel_->setText(tr("History"));

    auto entries = historyEntries_;
    std::sort(entries.begin(), entries.end(), [](const HistoryEntry &left, const HistoryEntry &right) {
        return left.lastOpenedAt > right.lastOpenedAt;
    });

    auto eagerThumbnailCount = 0;
    for (const auto &entry : entries) {
        if (!entry.bookPath.isEmpty()) {
            addHistoryEntry(entry);
            if (eagerThumbnailCount < maxEagerHistoryThumbnails) {
                loadHistoryThumbnailAsync(entry, requestId);
                ++eagerThumbnailCount;
            }
        }
    }

    updateNavigationButtons();
}

void Sidebar::populateSettingsPanel() {
    auto *layout = new QVBoxLayout(settingsPanel_);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(12);

    auto *formLayout = new QFormLayout();
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setFormAlignment(Qt::AlignTop);
    formLayout->setSpacing(8);

    auto *homeFolderRow = new QWidget(settingsPanel_);
    auto *homeFolderLayout = new QHBoxLayout(homeFolderRow);
    homeFolderLayout->setContentsMargins(0, 0, 0, 0);
    homeFolderLayout->setSpacing(6);
    homeFolderEdit_ = new QLineEdit(homeFolderRow);
    auto *browseButton = new QPushButton(tr("Browse"), homeFolderRow);
    homeFolderLayout->addWidget(homeFolderEdit_, 1);
    homeFolderLayout->addWidget(browseButton);
    formLayout->addRow(tr("Home folder"), homeFolderRow);

    defaultReadingDirectionCombo_ = new QComboBox(settingsPanel_);
    defaultReadingDirectionCombo_->addItem(tr("Right to left"), static_cast<int>(ReadingDirection::RightToLeft));
    defaultReadingDirectionCombo_->addItem(tr("Left to right"), static_cast<int>(ReadingDirection::LeftToRight));
    formLayout->addRow(tr("Default direction"), defaultReadingDirectionCombo_);

    defaultViewModeCombo_ = new QComboBox(settingsPanel_);
    defaultViewModeCombo_->addItem(tr("Single page"), static_cast<int>(ViewMode::SinglePage));
    defaultViewModeCombo_->addItem(tr("Spread"), static_cast<int>(ViewMode::Spread));
    formLayout->addRow(tr("Default view"), defaultViewModeCombo_);

    overlayEdgeTriggerSizeSpin_ = new QSpinBox(settingsPanel_);
    overlayEdgeTriggerSizeSpin_->setRange(1, 128);
    overlayEdgeTriggerSizeSpin_->setSuffix(tr(" px"));
    formLayout->addRow(tr("Edge trigger"), overlayEdgeTriggerSizeSpin_);

    overlayHideDelaySpin_ = new QSpinBox(settingsPanel_);
    overlayHideDelaySpin_->setRange(0, 5000);
    overlayHideDelaySpin_->setSingleStep(50);
    overlayHideDelaySpin_->setSuffix(tr(" ms"));
    formLayout->addRow(tr("Hide delay"), overlayHideDelaySpin_);

    pageLoadDebounceSpin_ = new QSpinBox(settingsPanel_);
    pageLoadDebounceSpin_->setRange(0, 2000);
    pageLoadDebounceSpin_->setSingleStep(10);
    pageLoadDebounceSpin_->setSuffix(tr(" ms"));
    formLayout->addRow(tr("Page load delay"), pageLoadDebounceSpin_);

    imageCacheMemoryLimitSpin_ = new QSpinBox(settingsPanel_);
    imageCacheMemoryLimitSpin_->setRange(1, 4096);
    imageCacheMemoryLimitSpin_->setSuffix(tr(" MiB"));
    formLayout->addRow(tr("Image cache"), imageCacheMemoryLimitSpin_);

    sidebarWidthSpin_ = new QSpinBox(settingsPanel_);
    sidebarWidthSpin_->setRange(minimumSidebarWidth, maximumSidebarWidth);
    sidebarWidthSpin_->setSuffix(tr(" px"));
    formLayout->addRow(tr("Sidebar width"), sidebarWidthSpin_);

    layout->addLayout(formLayout);
    layout->addStretch(1);

    connect(browseButton, &QPushButton::clicked, this, &Sidebar::browseHomeFolder);
    connect(homeFolderEdit_, &QLineEdit::editingFinished, this, &Sidebar::emitSettingsChanged);
    connect(defaultReadingDirectionCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &Sidebar::emitSettingsChanged);
    connect(defaultViewModeCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &Sidebar::emitSettingsChanged);
    connect(overlayEdgeTriggerSizeSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &Sidebar::emitSettingsChanged);
    connect(overlayHideDelaySpin_, qOverload<int>(&QSpinBox::valueChanged), this, &Sidebar::emitSettingsChanged);
    connect(pageLoadDebounceSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &Sidebar::emitSettingsChanged);
    connect(imageCacheMemoryLimitSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &Sidebar::emitSettingsChanged);
    connect(sidebarWidthSpin_, qOverload<int>(&QSpinBox::valueChanged), this, &Sidebar::emitSettingsChanged);

    setAppSettings(appSettings_);
}

void Sidebar::emitSettingsChanged() {
    if (updatingSettingsControls_) {
        return;
    }

    const auto requestedHomeFolder = homeFolderEdit_->text().trimmed();
    appSettings_.homeFolder =
        QFileInfo(requestedHomeFolder).isDir() ? QFileInfo(requestedHomeFolder).absoluteFilePath() : QDir::homePath();
    appSettings_.defaultReadingDirection =
        defaultReadingDirectionCombo_->currentData().toInt() == static_cast<int>(ReadingDirection::RightToLeft)
            ? ReadingDirection::RightToLeft
            : ReadingDirection::LeftToRight;
    appSettings_.defaultViewMode =
        defaultViewModeCombo_->currentData().toInt() == static_cast<int>(ViewMode::SinglePage) ? ViewMode::SinglePage
                                                                                               : ViewMode::Spread;
    appSettings_.overlayEdgeTriggerSize = overlayEdgeTriggerSizeSpin_->value();
    appSettings_.overlayHideDelayMs = overlayHideDelaySpin_->value();
    appSettings_.pageLoadDebounceMs = pageLoadDebounceSpin_->value();
    appSettings_.imageCacheMemoryLimitMiB = imageCacheMemoryLimitSpin_->value();
    appSettings_.sidebarWidth = sidebarWidthSpin_->value();

    emit appSettingsChanged(appSettings_);
}

void Sidebar::browseHomeFolder() {
    const auto selectedFolder =
        QFileDialog::getExistingDirectory(this, tr("Select home folder"), homeFolderEdit_->text());
    if (selectedFolder.isEmpty()) {
        return;
    }

    homeFolderEdit_->setText(QFileInfo(selectedFolder).absoluteFilePath());
    emitSettingsChanged();
}

void Sidebar::updateNavigationButtons() {
    const auto showingAlternateView = showingHistory_ || showingSettings_;
    backButton_->setEnabled(!showingAlternateView && !backStack_.isEmpty());
    forwardButton_->setEnabled(!showingAlternateView && !forwardStack_.isEmpty());
    upButton_->setEnabled(!showingAlternateView && QFileInfo(currentFolder_).dir().absolutePath() != currentFolder_);
}

void Sidebar::handleItemClicked(QListWidgetItem *item) {
    if (item == nullptr) {
        return;
    }

    const auto entryType = static_cast<EntryType>(item->data(entryTypeRole).toInt());
    const auto entryPath = item->data(entryPathRole).toString();
    const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());

    if (itemKind == SidebarItemKind::HistoryEntry) {
        clearPendingDirectoryClick();
        const auto bookType = static_cast<BookType>(item->data(bookTypeRole).toInt());
        if (bookType == BookType::Folder) {
            setCurrentFolder(entryPath);
            emit folderBookRequested(entryPath);
        } else {
            emit archiveBookRequested(entryPath);
        }
        return;
    }

    switch (entryType) {
    case EntryType::Directory:
        pendingDirectoryClickPath_ = entryPath;
        pendingDirectoryClickTimer_->start(std::max(folderSingleClickDelayMs, QApplication::doubleClickInterval()));
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
    const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());
    if (itemKind == SidebarItemKind::HistoryEntry) {
        return;
    }
    if (entryType != EntryType::Directory) {
        return;
    }

    clearPendingDirectoryClick();
    navigateToFolder(item->data(entryPathRole).toString());
}

void Sidebar::showFileListContextMenu(const QPoint &position) {
    clearPendingDirectoryClick();

    auto *item = fileList_->itemAt(position);
    if (item != nullptr) {
        fileList_->setCurrentItem(item);
    }

    const auto bookPath = historyBookPathForItem(item);
    const auto hasBookHistory = !bookPath.isEmpty() && historyEntryForPath(bookPath) != nullptr;

    QMenu menu(this);
    auto *deleteEntryAction = menu.addAction(tr("Delete history"));
    deleteEntryAction->setEnabled(hasBookHistory);
    auto *deleteCurrentFolderAction = menu.addAction(tr("Delete history in current folder"));
    deleteCurrentFolderAction->setEnabled(!currentFolder_.isEmpty());

    const auto *selectedAction = menu.exec(fileList_->viewport()->mapToGlobal(position));
    if (selectedAction == deleteEntryAction && hasBookHistory) {
        emit historyEntryDeleteRequested(bookPath);
    } else if (selectedAction == deleteCurrentFolderAction && !currentFolder_.isEmpty()) {
        emit currentFolderHistoryDeleteRequested(currentFolder_);
    }
}

bool Sidebar::showFileListItemToolTip(QHelpEvent *event) {
    if (event == nullptr || fileList_ == nullptr || fileList_->viewport() == nullptr) {
        return false;
    }

    const auto viewportPosition = fileList_->viewport()->mapFromGlobal(event->globalPos());
    auto *item = fileList_->itemAt(viewportPosition);
    if (item == nullptr) {
        return false;
    }

    const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());
    if (itemKind == SidebarItemKind::FileEntry) {
        const auto itemRect = visibleViewportRect(fileList_->visualItemRect(item), fileList_->viewport());
        const auto name = item->data(Qt::DisplayRole).toString();
        if (name.isEmpty()) {
            return false;
        }

        QToolTip::showText(event->globalPos() + QPoint(16, 16), name, fileList_->viewport(), itemRect);
        return true;
    }

    if (itemKind == SidebarItemKind::HistoryEntry) {
        const auto itemRect = visibleViewportRect(fileList_->visualItemRect(item), fileList_->viewport());
        const auto name = item->data(Qt::DisplayRole).toString();
        if (name.isEmpty()) {
            return false;
        }

        QToolTip::showText(event->globalPos() + QPoint(16, 16), name, fileList_->viewport(), itemRect);
        return true;
    }

    return false;
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
    auto *item = new QListWidgetItem(name, fileList_);
    const auto absolutePath = QFileInfo(path).absoluteFilePath();
    item->setData(itemKindRole, static_cast<int>(SidebarItemKind::FileEntry));
    item->setData(entryTypeRole, static_cast<int>(entryType));
    item->setData(entryPathRole, absolutePath);
    item->setData(readingStateRole, readingState(entryType, absolutePath));

    const auto isCurrentFolderBook = entryType == EntryType::Directory && !currentFolderBookPath_.isEmpty() &&
                                     absolutePath == currentFolderBookPath_;
    const auto isCurrentArchive =
        entryType == EntryType::Archive && !currentArchivePath_.isEmpty() && absolutePath == currentArchivePath_;
    if (isCurrentFolderBook || isCurrentArchive) {
        item->setSelected(true);
    }
}

void Sidebar::addHistoryEntry(const HistoryEntry &entry) {
    auto *item = new QListWidgetItem(
        entry.displayName.isEmpty() ? QFileInfo(entry.bookPath).fileName() : entry.displayName, fileList_);
    item->setData(itemKindRole, static_cast<int>(SidebarItemKind::HistoryEntry));
    item->setData(entryTypeRole,
                  static_cast<int>(entry.bookType == BookType::Folder ? EntryType::Directory : EntryType::Archive));
    item->setData(entryPathRole, QFileInfo(entry.bookPath).absoluteFilePath());
    item->setData(bookTypeRole, static_cast<int>(entry.bookType));
    item->setData(readingStateRole, static_cast<int>(readingStateForHistoryEntry(entry)));
    item->setData(secondaryTextRole, QFileInfo(entry.bookPath).absoluteFilePath());
    item->setData(progressTextRole, progressText(entry));
}

void Sidebar::loadHistoryThumbnailAsync(const HistoryEntry &entry, int requestId) {
    auto *watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, requestId, bookPath = entry.bookPath] {
        const auto image = watcher->result();
        watcher->deleteLater();

        if (requestId != historyThumbnailGeneration_ || !showingHistory_ || image.isNull()) {
            return;
        }

        const auto normalizedPath = QFileInfo(bookPath).absoluteFilePath();
        for (int row = 0; row < fileList_->count(); ++row) {
            auto *item = fileList_->item(row);
            if (item == nullptr) {
                continue;
            }
            const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());
            if (itemKind == SidebarItemKind::HistoryEntry && item->data(entryPathRole).toString() == normalizedPath) {
                item->setData(thumbnailImageRole, image);
                fileList_->update(fileList_->model()->index(row, 0));
                return;
            }
        }
    });
    watcher->setFuture(QtConcurrent::run(loadFirstPageThumbnail, entry));
}

void Sidebar::updateFileListReadingStates() {
    for (int row = 0; row < fileList_->count(); ++row) {
        auto *item = fileList_->item(row);
        if (item == nullptr) {
            continue;
        }
        const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());
        if (itemKind != SidebarItemKind::FileEntry) {
            continue;
        }

        const auto entryType = static_cast<EntryType>(item->data(entryTypeRole).toInt());
        const auto entryPath = item->data(entryPathRole).toString();
        item->setData(readingStateRole, readingState(entryType, entryPath));
    }
    fileList_->viewport()->update();
}

void Sidebar::updateCurrentBookSelection() {
    if (!currentArchivePath_.isEmpty()) {
        selectFileListEntry(currentArchivePath_, true);
        return;
    }

    selectFileListEntry(currentFolderBookPath_, true);
}

QListWidgetItem *Sidebar::fileListEntryForPath(const QString &path) const {
    if (path.isEmpty()) {
        return nullptr;
    }

    const auto normalizedPath = QFileInfo(path).absoluteFilePath();
    for (int row = 0; row < fileList_->count(); ++row) {
        auto *item = fileList_->item(row);
        if (item == nullptr) {
            continue;
        }
        const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());
        const auto entryPath = item->data(entryPathRole).toString();
        if (itemKind == SidebarItemKind::FileEntry && entryPath == normalizedPath) {
            return item;
        }
    }

    return nullptr;
}

bool Sidebar::selectFileListEntry(const QString &path, bool scrollToItem) {
    const QSignalBlocker blocker(fileList_);
    fileList_->clearSelection();

    auto *item = fileListEntryForPath(path);
    if (item == nullptr) {
        fileList_->setCurrentRow(-1);
        fileList_->viewport()->update();
        return false;
    }

    fileList_->setCurrentItem(item);
    item->setSelected(true);
    if (scrollToItem) {
        fileList_->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    }
    fileList_->viewport()->update();
    return true;
}

void Sidebar::applySortSettingsForCurrentFolder() {
    const auto normalized = currentFolder_.isEmpty() ? QString() : QFileInfo(currentFolder_).absoluteFilePath();
    const auto settings = appSettings_.sidebarFolderSorts.value(normalized);
    sortKey_ = settings.key;
    sortOrder_ = settings.order;
    updateSortButtonText();
}

void Sidebar::saveSortSettingsForCurrentFolder() {
    if (currentFolder_.isEmpty()) {
        return;
    }

    const auto normalized = QFileInfo(currentFolder_).absoluteFilePath();
    if (sortKey_ == SidebarSortKey::FileName && sortOrder_ == SidebarSortOrder::Ascending) {
        appSettings_.sidebarFolderSorts.remove(normalized);
    } else {
        appSettings_.sidebarFolderSorts.insert(normalized, SidebarSortSettings{sortKey_, sortOrder_});
    }
    emit appSettingsChanged(appSettings_);
}

void Sidebar::updateSortButtonText() {
    QString keyText;
    switch (sortKey_) {
    case SidebarSortKey::FileName:
        keyText = tr("Filename");
        break;
    case SidebarSortKey::CreatedAt:
        keyText = tr("Created");
        break;
    case SidebarSortKey::ModifiedAt:
        keyText = tr("Modified");
        break;
    }

    const auto arrow = sortOrder_ == SidebarSortOrder::Ascending ? QStringLiteral("↑") : QStringLiteral("↓");
    sortButton_->setText(QStringLiteral("%1 %2").arg(arrow, keyText));
}

QString Sidebar::historyBookPathForItem(const QListWidgetItem *item) const {
    if (item == nullptr) {
        return {};
    }

    const auto itemKind = static_cast<SidebarItemKind>(item->data(itemKindRole).toInt());
    const auto entryPath = item->data(entryPathRole).toString();
    if (entryPath.isEmpty()) {
        return {};
    }

    if (itemKind == SidebarItemKind::HistoryEntry) {
        return QFileInfo(entryPath).absoluteFilePath();
    }

    const auto entryType = static_cast<EntryType>(item->data(entryTypeRole).toInt());
    switch (entryType) {
    case EntryType::Directory:
    case EntryType::Archive:
        return QFileInfo(entryPath).absoluteFilePath();
    case EntryType::Image:
        return QFileInfo(entryPath).dir().absolutePath();
    }
    return {};
}

int Sidebar::readingState(EntryType entryType, const QString &path) const {
    QString bookPath;
    if (entryType == EntryType::Image) {
        bookPath = QFileInfo(path).dir().absolutePath();
    } else if (entryType == EntryType::Archive) {
        bookPath = QFileInfo(path).absoluteFilePath();
    } else if (entryType == EntryType::Directory) {
        bookPath = QFileInfo(path).absoluteFilePath();
    } else {
        return static_cast<int>(ReadingState::Unread);
    }

    const auto *entry = historyEntryForPath(bookPath);
    if (entry == nullptr) {
        return static_cast<int>(ReadingState::Unread);
    }
    if (entry->pageCount <= 0) {
        return static_cast<int>(ReadingState::Unread);
    }
    if (entry->lastPageIndex >= entry->pageCount - 1) {
        return static_cast<int>(ReadingState::Completed);
    }
    if (entry->lastPageIndex > 0) {
        return static_cast<int>(ReadingState::Reading);
    }
    return static_cast<int>(ReadingState::Unread);
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

void Sidebar::updateResizeCursor(const QPoint &position) {
    if (resizing_ || isResizeHandlePosition(position)) {
        setCursor(Qt::SizeHorCursor);
        return;
    }

    unsetCursor();
}

bool Sidebar::isResizeHandlePosition(const QPoint &position) const {
    return position.x() >= width() - resizeHandleWidth;
}

} // namespace weeview
