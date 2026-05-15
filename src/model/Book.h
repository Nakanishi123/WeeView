#pragma once

#include "model/CoreTypes.h"

#include <QImage>
#include <QString>

namespace weeview {

class Book {
  public:
    virtual ~Book() = default;

    [[nodiscard]] virtual BookType type() const = 0;
    [[nodiscard]] virtual QString displayName() const = 0;
    [[nodiscard]] virtual QString sourcePath() const = 0;
    [[nodiscard]] virtual int pageCount() const = 0;
    [[nodiscard]] virtual PageInfo pageInfo(int pageIndex) const = 0;
    [[nodiscard]] virtual PageInfo loadPageInfo(int pageIndex) const = 0;
    [[nodiscard]] virtual QImage loadPage(int pageIndex) const = 0;
};

} // namespace weeview
