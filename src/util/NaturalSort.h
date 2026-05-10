#pragma once

#include <QString>
#include <QStringList>

namespace weeview::naturalsort {

bool lessThan(const QString &left, const QString &right);
void sort(QStringList &values);
QStringList sorted(QStringList values);

} // namespace weeview::naturalsort
