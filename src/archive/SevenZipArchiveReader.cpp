#include "SevenZipArchiveReader.h"

#include <archive.h>
#include <archive_entry.h>

#include <QFile>

#include <algorithm>
#include <limits>
#include <utility>

namespace weeview {
namespace {

constexpr qsizetype archiveBlockSize = 10240;

QString archiveEntryPath(archive_entry *entry) {
    if (entry == nullptr) {
        return {};
    }

    if (const auto *path = archive_entry_pathname_utf8(entry); path != nullptr) {
        return QString::fromUtf8(path);
    }
    if (const auto *path = archive_entry_pathname(entry); path != nullptr) {
        return QString::fromLocal8Bit(path);
    }
    return {};
}

bool isDirectoryEntry(archive_entry *entry, const QString &path) {
    return archive_entry_filetype(entry) == AE_IFDIR || path.endsWith(QLatin1Char('/'));
}

} // namespace

SevenZipArchiveReader::SevenZipArchiveReader(QString archivePath) : archivePath_(std::move(archivePath)) {
    canOpen_ = openArchive() != nullptr;
}

SevenZipArchiveReader::~SevenZipArchiveReader() = default;

SevenZipArchiveReader::SevenZipArchiveReader(SevenZipArchiveReader &&) noexcept = default;

SevenZipArchiveReader &SevenZipArchiveReader::operator=(SevenZipArchiveReader &&) noexcept = default;

bool SevenZipArchiveReader::isOpen() const { return canOpen_; }

QVector<ArchiveEntry> SevenZipArchiveReader::entries() const {
    auto archiveReader = openArchive();
    if (!archiveReader) {
        return {};
    }

    QVector<ArchiveEntry> result;
    archive_entry *entry = nullptr;
    while (archive_read_next_header(archiveReader.get(), &entry) == ARCHIVE_OK) {
        const auto path = archiveEntryPath(entry);
        if (!path.isEmpty() && !isDirectoryEntry(entry, path)) {
            const auto size = archive_entry_size(entry);
            result.append({
                path,
                size < 0 ? 0
                         : static_cast<qsizetype>(std::min<la_int64_t>(size, std::numeric_limits<qsizetype>::max())),
            });
        }
        archive_read_data_skip(archiveReader.get());
    }

    return result;
}

QByteArray SevenZipArchiveReader::readFile(const QString &entryPath) const {
    auto archiveReader = openArchive();
    if (!archiveReader) {
        return {};
    }

    archive_entry *entry = nullptr;
    while (archive_read_next_header(archiveReader.get(), &entry) == ARCHIVE_OK) {
        if (archiveEntryPath(entry) != entryPath) {
            archive_read_data_skip(archiveReader.get());
            continue;
        }

        QByteArray data;
        const void *block = nullptr;
        size_t size = 0;
        la_int64_t offset = 0;
        while (true) {
            const auto status = archive_read_data_block(archiveReader.get(), &block, &size, &offset);
            if (status == ARCHIVE_EOF) {
                return data;
            }
            if (status != ARCHIVE_OK) {
                return {};
            }
            data.append(static_cast<const char *>(block), static_cast<qsizetype>(size));
        }
    }

    return {};
}

void SevenZipArchiveReader::ArchiveDeleter::operator()(archive *archiveReader) const {
    if (archiveReader != nullptr) {
        archive_read_free(archiveReader);
    }
}

std::unique_ptr<archive, SevenZipArchiveReader::ArchiveDeleter> SevenZipArchiveReader::openArchive() const {
    std::unique_ptr<archive, ArchiveDeleter> archiveReader(archive_read_new());
    if (!archiveReader) {
        return {};
    }

    archive_read_support_format_7zip(archiveReader.get());

    const auto encodedArchivePath = QFile::encodeName(archivePath_);
    if (archive_read_open_filename(archiveReader.get(), encodedArchivePath.constData(), archiveBlockSize) !=
        ARCHIVE_OK) {
        return {};
    }

    return archiveReader;
}

} // namespace weeview
