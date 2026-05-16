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
constexpr la_int64_t maxReadableEntrySize = 512LL * 1024LL * 1024LL;
constexpr qsizetype maxExtractedFileCacheBytes = 64 * 1024 * 1024;

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
    if (const auto cached = cachedFile(entryPath); !cached.isNull()) {
        return cached;
    }

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

        const auto expectedSize = archive_entry_size(entry);
        if (expectedSize > maxReadableEntrySize) {
            return {};
        }

        QByteArray data;
        if (expectedSize >= 0) {
            data.reserve(static_cast<qsizetype>(std::min<la_int64_t>(expectedSize, maxReadableEntrySize)));
        }

        const void *block = nullptr;
        size_t size = 0;
        la_int64_t offset = 0;
        while (true) {
            const auto status = archive_read_data_block(archiveReader.get(), &block, &size, &offset);
            if (status == ARCHIVE_EOF) {
                cacheFile(entryPath, data);
                return data;
            }
            if (status != ARCHIVE_OK) {
                return {};
            }
            if (size > static_cast<size_t>(std::numeric_limits<qsizetype>::max()) ||
                data.size() > std::numeric_limits<qsizetype>::max() - static_cast<qsizetype>(size)) {
                return {};
            }

            const auto nextSize = data.size() + static_cast<qsizetype>(size);
            if (nextSize > maxReadableEntrySize || (expectedSize >= 0 && nextSize > expectedSize)) {
                return {};
            }

            data.append(static_cast<const char *>(block), static_cast<qsizetype>(size));
        }
    }

    return {};
}

QByteArray SevenZipArchiveReader::cachedFile(const QString &entryPath) const {
    const auto it = extractedFileCache_.constFind(entryPath);
    if (it == extractedFileCache_.constEnd()) {
        return {};
    }
    extractedFileCacheOrder_.removeAll(entryPath);
    extractedFileCacheOrder_.append(entryPath);
    return it.value();
}

void SevenZipArchiveReader::cacheFile(const QString &entryPath, const QByteArray &data) const {
    if (data.isEmpty() || data.size() > maxExtractedFileCacheBytes) {
        return;
    }

    if (const auto it = extractedFileCache_.find(entryPath); it != extractedFileCache_.end()) {
        extractedFileCacheBytes_ -= it.value().size();
        extractedFileCache_.erase(it);
        extractedFileCacheOrder_.removeAll(entryPath);
    }

    extractedFileCache_.insert(entryPath, data);
    extractedFileCacheOrder_.append(entryPath);
    extractedFileCacheBytes_ += data.size();

    while (extractedFileCacheBytes_ > maxExtractedFileCacheBytes && !extractedFileCacheOrder_.isEmpty()) {
        const auto oldestEntryPath = extractedFileCacheOrder_.takeFirst();
        const auto it = extractedFileCache_.find(oldestEntryPath);
        if (it == extractedFileCache_.end()) {
            continue;
        }

        extractedFileCacheBytes_ -= it.value().size();
        extractedFileCache_.erase(it);
    }
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
