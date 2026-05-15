#include "ZipArchiveReader.h"

#include <zip.h>

#include <QFile>
#include <algorithm>
#include <limits>
#include <utility>

namespace weeview {
namespace {

constexpr zip_flags_t utf8NameFlags = ZIP_FL_ENC_UTF_8;

} // namespace

ZipArchiveReader::ZipArchiveReader(QString archivePath) : archivePath_(std::move(archivePath)) {
    int errorCode = 0;
    const auto encodedArchivePath = QFile::encodeName(archivePath_);
    archive_.reset(zip_open(encodedArchivePath.constData(), ZIP_RDONLY, &errorCode));
}

ZipArchiveReader::~ZipArchiveReader() = default;

ZipArchiveReader::ZipArchiveReader(ZipArchiveReader &&) noexcept = default;

ZipArchiveReader &ZipArchiveReader::operator=(ZipArchiveReader &&) noexcept = default;

bool ZipArchiveReader::isOpen() const { return archive_ != nullptr; }

QVector<ArchiveEntry> ZipArchiveReader::entries() const {
    if (!archive_) {
        return {};
    }

    const auto entryCount = zip_get_num_entries(archive_.get(), 0);
    if (entryCount <= 0) {
        return {};
    }

    QVector<ArchiveEntry> result;
    result.reserve(static_cast<qsizetype>(entryCount));

    for (zip_uint64_t index = 0; index < static_cast<zip_uint64_t>(entryCount); ++index) {
        const auto path = entryName(static_cast<qsizetype>(index));
        if (path.isEmpty() || path.endsWith(QLatin1Char('/'))) {
            continue;
        }

        zip_stat_t stat;
        zip_stat_init(&stat);
        if (zip_stat_index(archive_.get(), index, 0, &stat) != 0) {
            continue;
        }

        result.append({
            path,
            static_cast<qsizetype>(std::min<zip_uint64_t>(stat.size, std::numeric_limits<qsizetype>::max())),
        });
    }

    return result;
}

QByteArray ZipArchiveReader::readFile(const QString &entryPath) const {
    if (!archive_) {
        return {};
    }

    auto entryIndex = zip_name_locate(archive_.get(), entryPath.toUtf8().constData(), utf8NameFlags);
    if (entryIndex < 0) {
        entryIndex = zip_name_locate(archive_.get(), entryPath.toUtf8().constData(), 0);
    }
    if (entryIndex < 0) {
        return {};
    }

    zip_stat_t stat;
    zip_stat_init(&stat);
    if (zip_stat_index(archive_.get(), static_cast<zip_uint64_t>(entryIndex), 0, &stat) != 0) {
        return {};
    }

    std::unique_ptr<zip_file_t, decltype(&zip_fclose)> file(
        zip_fopen_index(archive_.get(), static_cast<zip_uint64_t>(entryIndex), 0), &zip_fclose);
    if (!file) {
        return {};
    }

    QByteArray data;
    data.resize(static_cast<qsizetype>(std::min<zip_uint64_t>(stat.size, std::numeric_limits<qsizetype>::max())));

    qsizetype offset = 0;
    while (offset < data.size()) {
        const auto bytesRead = zip_fread(file.get(), data.data() + offset, data.size() - offset);
        if (bytesRead < 0) {
            return {};
        }
        if (bytesRead == 0) {
            break;
        }
        offset += bytesRead;
    }

    data.truncate(offset);
    return data;
}

void ZipArchiveReader::ZipDeleter::operator()(zip *archive) const {
    if (archive != nullptr) {
        zip_discard(archive);
    }
}

QString ZipArchiveReader::entryName(qsizetype entryIndex) const {
    if (!archive_) {
        return {};
    }

    const auto *name = zip_get_name(archive_.get(), static_cast<zip_uint64_t>(entryIndex), utf8NameFlags);
    if (name == nullptr) {
        name = zip_get_name(archive_.get(), static_cast<zip_uint64_t>(entryIndex), 0);
    }
    return name == nullptr ? QString() : QString::fromUtf8(name);
}

} // namespace weeview
