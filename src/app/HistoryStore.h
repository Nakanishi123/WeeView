#pragma once

#include "model/CoreTypes.h"

#include <QString>
#include <QVector>

namespace weeview {

class HistoryStore {
  public:
    HistoryStore();
    explicit HistoryStore(QString filePath);

    [[nodiscard]] QVector<HistoryEntry> load() const;
    [[nodiscard]] bool save(const QVector<HistoryEntry> &entries) const;
    [[nodiscard]] QString filePath() const;

  private:
    QString filePath_;
};

} // namespace weeview
