#include "FooterBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSlider>

#include <algorithm>

namespace weeview {

FooterBar::FooterBar(QWidget *parent) : QWidget(parent) {
    setAutoFillBackground(true);
    setStyleSheet(QStringLiteral("FooterBar { background: rgba(24, 24, 24, 220); color: white; }"));

    pageCounterLabel_ = new QLabel(this);
    pageCounterLabel_->setMinimumWidth(72);

    pageSlider_ = new QSlider(Qt::Horizontal, this);
    pageSlider_->setTracking(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(10);
    layout->addWidget(pageCounterLabel_);
    layout->addWidget(pageSlider_, 1);

    connect(pageSlider_, &QSlider::valueChanged, this, [this](int value) {
        if (currentPageIndex_ == value) {
            return;
        }

        currentPageIndex_ = value;
        updatePageControls();
        emit currentPageIndexChanged(currentPageIndex_);
    });

    updatePageControls();
}

void FooterBar::setPageCount(int pageCount) {
    pageCount_ = std::max(0, pageCount);
    currentPageIndex_ = pageCount_ == 0 ? 0 : std::clamp(currentPageIndex_, 0, pageCount_ - 1);
    updatePageControls();
}

void FooterBar::setCurrentPageIndex(int pageIndex) {
    const auto nextPageIndex = pageCount_ == 0 ? 0 : std::clamp(pageIndex, 0, pageCount_ - 1);
    if (currentPageIndex_ == nextPageIndex) {
        return;
    }

    currentPageIndex_ = nextPageIndex;
    updatePageControls();
}

void FooterBar::setReadingDirection(ReadingDirection readingDirection) {
    if (readingDirection_ == readingDirection) {
        return;
    }

    readingDirection_ = readingDirection;
    updatePageControls();
}

int FooterBar::pageCount() const { return pageCount_; }

int FooterBar::currentPageIndex() const { return currentPageIndex_; }

ReadingDirection FooterBar::readingDirection() const { return readingDirection_; }

void FooterBar::updatePageControls() {
    pageCounterLabel_->setText(
        QStringLiteral("%1 / %2").arg(pageCount_ == 0 ? 0 : currentPageIndex_ + 1).arg(pageCount_));

    const QSignalBlocker blocker(pageSlider_);
    pageSlider_->setEnabled(pageCount_ > 0);
    pageSlider_->setRange(0, std::max(0, pageCount_ - 1));
    pageSlider_->setValue(currentPageIndex_);
    pageSlider_->setInvertedAppearance(readingDirection_ == ReadingDirection::RightToLeft);
}

} // namespace weeview
