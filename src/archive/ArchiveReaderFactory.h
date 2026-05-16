#pragma once

#include "archive/ArchiveReader.h"

#include <QString>

#include <memory>

namespace weeview {

[[nodiscard]] std::unique_ptr<ArchiveReader> createArchiveReader(const QString &archivePath);

} // namespace weeview
