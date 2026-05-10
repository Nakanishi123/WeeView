#include "NaturalSort.h"

#include <QCollator>

#include <algorithm>

namespace weeview::naturalsort {
namespace {

QCollator makeCollator() {
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    return collator;
}

} // namespace

bool lessThan(const QString &left, const QString &right) {
    auto collator = makeCollator();
    return collator.compare(left, right) < 0;
}

void sort(QStringList &values) {
    auto collator = makeCollator();
    std::sort(values.begin(), values.end(),
              [&collator](const QString &left, const QString &right) {
                  return collator.compare(left, right) < 0;
              });
}

QStringList sorted(QStringList values) {
    sort(values);
    return values;
}

} // namespace weeview::naturalsort
