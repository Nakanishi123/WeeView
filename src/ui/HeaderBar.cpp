#include "HeaderBar.h"

#include "IconUtils.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>
#include <QStyle>
#include <QWindow>

namespace weeview {
namespace {

constexpr int headerButtonSize = 39;
constexpr int headerIconSize = 25;
const QColor iconColor(245, 245, 245);

void configureIconButton(QPushButton *button, const QString &label) {
    button->setIconSize(QSize(headerIconSize, headerIconSize));
    button->setToolTip(label);
    button->setAccessibleName(label);
    button->setFixedSize(headerButtonSize, headerButtonSize);
    button->setFocusPolicy(Qt::NoFocus);
}

void configureWindowButton(QPushButton *button, const QString &label, QStyle::StandardPixmap icon) {
    button->setIcon(button->style()->standardIcon(icon));
    button->setIconSize(QSize(18, 18));
    button->setToolTip(label);
    button->setAccessibleName(label);
    button->setFixedSize(headerButtonSize, headerButtonSize);
    button->setFocusPolicy(Qt::NoFocus);
}

} // namespace

HeaderBar::HeaderBar(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral("HeaderBar { background: rgba(24, 24, 24, 220); color: white; }"
                                 "QPushButton { padding: 4px 10px; }"));

    bookPathLabel_ = new QLabel(this);
    bookPathLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bookPathLabel_->setTextFormat(Qt::PlainText);

    viewModeButton_ = new QPushButton(this);
    readingDirectionButton_ = new QPushButton(this);
    minimizeButton_ = new QPushButton(this);
    maximizeRestoreButton_ = new QPushButton(this);
    closeButton_ = new QPushButton(this);
    viewModeButton_->setCheckable(true);
    readingDirectionButton_->setCheckable(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 0, 0);
    layout->setSpacing(8);
    layout->addWidget(bookPathLabel_, 1);
    layout->addWidget(viewModeButton_);
    layout->addWidget(readingDirectionButton_);
    layout->addSpacing(8);
    layout->addWidget(minimizeButton_);
    layout->addWidget(maximizeRestoreButton_);
    layout->addWidget(closeButton_);

    connect(viewModeButton_, &QPushButton::clicked, this, &HeaderBar::toggleViewMode);
    connect(readingDirectionButton_, &QPushButton::clicked, this, &HeaderBar::toggleReadingDirection);
    connect(minimizeButton_, &QPushButton::clicked, this, &HeaderBar::minimizeRequested);
    connect(maximizeRestoreButton_, &QPushButton::clicked, this, &HeaderBar::maximizeRestoreRequested);
    connect(closeButton_, &QPushButton::clicked, this, &HeaderBar::closeRequested);

    updateButtons();
    updateWindowButtons();
}

void HeaderBar::setBookPath(const QString &bookPath) {
    bookPath_ = bookPath;
    bookPathLabel_->setText(bookPath_);
}

void HeaderBar::setViewMode(ViewMode viewMode) {
    if (viewMode_ == viewMode) {
        return;
    }

    viewMode_ = viewMode;
    updateButtons();
}

void HeaderBar::setReadingDirection(ReadingDirection readingDirection) {
    if (readingDirection_ == readingDirection) {
        return;
    }

    readingDirection_ = readingDirection;
    updateButtons();
}

void HeaderBar::setWindowMaximized(bool maximized) {
    if (windowMaximized_ == maximized) {
        return;
    }

    windowMaximized_ = maximized;
    updateWindowButtons();
}

QString HeaderBar::bookPath() const { return bookPath_; }

ViewMode HeaderBar::viewMode() const { return viewMode_; }

ReadingDirection HeaderBar::readingDirection() const { return readingDirection_; }

void HeaderBar::updateButtons() {
    const auto viewModeLabel = viewMode_ == ViewMode::SinglePage ? tr("Single page view") : tr("Spread page view");
    viewModeButton_->setIcon(icons::tintedSvgIcon(QStringLiteral(":/assets/book.svg"), iconColor, headerIconSize, 0));
    viewModeButton_->setChecked(viewMode_ == ViewMode::Spread);
    configureIconButton(viewModeButton_, viewModeLabel);

    const auto readingDirectionLabel =
        readingDirection_ == ReadingDirection::RightToLeft ? tr("Right-to-left reading") : tr("Left-to-right reading");
    readingDirectionButton_->setIcon(icons::tintedSvgIcon(readingDirection_ == ReadingDirection::RightToLeft
                                                              ? QStringLiteral(":/assets/chevron_left_circle.svg")
                                                              : QStringLiteral(":/assets/chevron_right_circle.svg"),
                                                          iconColor, headerIconSize, 0));
    readingDirectionButton_->setChecked(readingDirection_ == ReadingDirection::LeftToRight);
    configureIconButton(readingDirectionButton_, readingDirectionLabel);
}

void HeaderBar::updateWindowButtons() {
    configureWindowButton(minimizeButton_, tr("Minimize"), QStyle::SP_TitleBarMinButton);
    configureWindowButton(maximizeRestoreButton_, windowMaximized_ ? tr("Restore") : tr("Maximize"),
                          windowMaximized_ ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton);
    configureWindowButton(closeButton_, tr("Close"), QStyle::SP_TitleBarCloseButton);
}

void HeaderBar::toggleViewMode() {
    viewMode_ = viewMode_ == ViewMode::SinglePage ? ViewMode::Spread : ViewMode::SinglePage;
    updateButtons();
    emit viewModeChanged(viewMode_);
}

void HeaderBar::toggleReadingDirection() {
    readingDirection_ = readingDirection_ == ReadingDirection::RightToLeft ? ReadingDirection::LeftToRight
                                                                           : ReadingDirection::RightToLeft;
    updateButtons();
    emit readingDirectionChanged(readingDirection_);
}

void HeaderBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit maximizeRestoreRequested();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void HeaderBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (auto *handle = window()->windowHandle(); handle != nullptr) {
            handle->startSystemMove();
            event->accept();
            return;
        }
    }

    QWidget::mousePressEvent(event);
}

} // namespace weeview
