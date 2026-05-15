#pragma once

#include "app/AppSettings.h"

#include <QString>

namespace weeview {

class AppSettingsStore {
  public:
    AppSettingsStore();
    explicit AppSettingsStore(QString filePath);

    [[nodiscard]] AppSettings load() const;
    [[nodiscard]] bool save(const AppSettings &settings) const;
    [[nodiscard]] QString filePath() const;

  private:
    QString filePath_;
};

} // namespace weeview
