#include "NaturalSort.h"

#include <QChar>
#include <QCollator>

#include <algorithm>
#include <optional>

namespace weeview::naturalsort {
namespace {

QCollator makeCollator() {
    QCollator collator;
    collator.setNumericMode(true);
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    return collator;
}

std::optional<int> japaneseDigitValue(QChar character) {
    switch (character.unicode()) {
    case u'〇':
    case u'零':
        return 0;
    case u'一':
        return 1;
    case u'二':
        return 2;
    case u'三':
        return 3;
    case u'四':
        return 4;
    case u'五':
        return 5;
    case u'六':
        return 6;
    case u'七':
        return 7;
    case u'八':
        return 8;
    case u'九':
        return 9;
    default:
        return std::nullopt;
    }
}

int japaneseSmallUnitValue(QChar character) {
    switch (character.unicode()) {
    case u'十':
        return 10;
    case u'百':
        return 100;
    case u'千':
        return 1000;
    default:
        return 0;
    }
}

bool isJapaneseNumberCharacter(QChar character) {
    return japaneseDigitValue(character).has_value() || japaneseSmallUnitValue(character) != 0 ||
           character == QChar(u'万');
}

qulonglong parseJapaneseNumber(QStringView text) {
    qulonglong total = 0;
    qulonglong section = 0;
    qulonglong current = 0;

    for (const auto character : text) {
        if (const auto digit = japaneseDigitValue(character)) {
            current = static_cast<qulonglong>(*digit);
            continue;
        }

        if (const auto unit = japaneseSmallUnitValue(character); unit != 0) {
            section += (current == 0 ? 1 : current) * static_cast<qulonglong>(unit);
            current = 0;
            continue;
        }

        if (character == QChar(u'万')) {
            section += current;
            total += (section == 0 ? 1 : section) * 10000;
            section = 0;
            current = 0;
        }
    }

    return total + section + current;
}

QString paddedNumber(qulonglong number) { return QString::number(number).rightJustified(20, QLatin1Char('0')); }

QString normalizedForSort(const QString &value) {
    QString normalized;
    normalized.reserve(value.size());

    for (qsizetype index = 0; index < value.size();) {
        const auto character = value.at(index);
        if (!isJapaneseNumberCharacter(character)) {
            normalized.append(character);
            ++index;
            continue;
        }

        const auto start = index;
        while (index < value.size() && isJapaneseNumberCharacter(value.at(index))) {
            ++index;
        }

        normalized.append(QLatin1Char('\x01'));
        normalized.append(paddedNumber(parseJapaneseNumber(QStringView(value).mid(start, index - start))));
        normalized.append(QLatin1Char('\x02'));
    }

    return normalized;
}

int compareNatural(const QString &left, const QString &right, QCollator &collator) {
    const auto leftKey = normalizedForSort(left);
    const auto rightKey = normalizedForSort(right);
    const auto result = collator.compare(leftKey, rightKey);
    if (result != 0) {
        return result;
    }
    return collator.compare(left, right);
}

} // namespace

bool lessThan(const QString &left, const QString &right) {
    auto collator = makeCollator();
    return compareNatural(left, right, collator) < 0;
}

void sort(QStringList &values) {
    auto collator = makeCollator();
    std::sort(values.begin(), values.end(), [&collator](const QString &left, const QString &right) {
        return compareNatural(left, right, collator) < 0;
    });
}

QStringList sorted(QStringList values) {
    sort(values);
    return values;
}

} // namespace weeview::naturalsort
