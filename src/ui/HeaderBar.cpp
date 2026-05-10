#include "HeaderBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>

namespace weeview {

HeaderBar::HeaderBar(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral("HeaderBar { background: rgba(24, 24, 24, 220); color: white; }"
                                 "QPushButton { padding: 4px 10px; }"));

    bookPathLabel_ = new QLabel(this);
    bookPathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    bookPathLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    bookPathLabel_->setTextFormat(Qt::PlainText);

    viewModeButton_ = new QPushButton(this);
    readingDirectionButton_ = new QPushButton(this);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);
    layout->addWidget(bookPathLabel_, 1);
    layout->addWidget(viewModeButton_);
    layout->addWidget(readingDirectionButton_);

    connect(viewModeButton_, &QPushButton::clicked, this, &HeaderBar::toggleViewMode);
    connect(readingDirectionButton_, &QPushButton::clicked, this, &HeaderBar::toggleReadingDirection);

    updateButtons();
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

QString HeaderBar::bookPath() const { return bookPath_; }

ViewMode HeaderBar::viewMode() const { return viewMode_; }

ReadingDirection HeaderBar::readingDirection() const { return readingDirection_; }

void HeaderBar::updateButtons() {
    viewModeButton_->setText(viewMode_ == ViewMode::SinglePage ? QStringLiteral("Single") : QStringLiteral("Spread"));
    readingDirectionButton_->setText(readingDirection_ == ReadingDirection::RightToLeft ? QStringLiteral("RTL")
                                                                                        : QStringLiteral("LTR"));
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

} // namespace weeview
