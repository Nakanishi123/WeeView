# WeeView Architecture

This document describes implementation structure, ownership rules, and module boundaries.

Behavioral requirements live under [`spec.md`](./spec.md). If this document conflicts with the behavior specification, report the conflict before changing behavior.

## Implementation rules

- Use C++23.
- Use CMake.
- Use Qt 6 Widgets.
- Do not use QML.
- Do not use Qt Designer `.ui` files.
- Store settings and history as JSON.
- Save JSON safely.
- Use RAII for external C resources.
- Wrap libzip handles in RAII classes.

## Ownership rules

- Prefer `std::unique_ptr` for exclusive ownership.
- Use `std::shared_ptr` only when shared ownership is unavoidable.
- Raw owning pointers are prohibited.
- Raw non-owning pointers are allowed.
- QObject/QWidget instances should usually be owned by Qt parent-child ownership.
- Do not give the same QObject/QWidget both a Qt parent and a smart pointer owner.
- Non-QObject services and models should use smart pointers.

## Suggested source layout

```text
src/
  app/        settings, history, application services
  archive/    archive reader abstractions and archive implementations
  image/      image decoder and image cache
  model/      book/page models
  ui/         widgets and overlays
  util/       natural sort, file type helpers
  viewer/     viewer state and page layout
```

## Core types

Keep these types small and easy to serialize where applicable.

- `ViewMode`: single page or spread.
- `ReadingDirection`: right-to-left or left-to-right.
- `BookType`: folder or archive.
- `PageInfo`: image name, display path, image size, and landscape flag.
- `ViewerState`: current page index, current display group end, view mode, reading direction, and spread group direction.
- `HistoryEntry`: persisted reading progress for a book.

Do not add persistent `spreadAnchorPage` or spread anchor parity. Use `currentPageIndex` as the logical first page index of the current directional local group.

## Book model

`Book` represents a readable collection of pages.

Required responsibilities:

- Return display name.
- Return source path.
- Return page count.
- Return page metadata.
- Load a page image.

MVP implementations:

- `FolderBook`
- `ArchiveBook`

## Archive layer

Use an archive abstraction so future archive formats can be added without changing viewer behavior.

MVP implementation:

- `ZipArchiveReader`, backed by libzip.
- `SevenZipArchiveReader`, backed by libarchive.

RAR is not part of MVP.

## Image decoder layer

The decoder layer should hide image backend details from the viewer.

MVP requirements:

- JPEG.
- PNG.
- WebP.
- AVIF.

The viewer should not care whether AVIF is handled by Qt image plugins or a dedicated backend.

## Image cache

Cache decoded images, not UI pixmaps.

Recommended MVP design:

- Key by page index.
- Store decoded images.
- Keep a memory-size limit based on decoded image bytes.
- Protect the current display group from eviction.
- Use an LRU-style fallback when memory must be reclaimed.

## UI architecture

Suggested high-level structure:

- `MainWindow`: owns and wires major UI components.
- `OverlayContainer`: positions the viewer and overlays.
- `MangaView`: renders pages.
- `HeaderBar`: top overlay controls.
- `FooterBar`: page status and slider.
- `Sidebar`: file browser.

QObject/QWidget lifetime should normally be handled through Qt parent-child ownership.

## Persistence layer

Settings and history are separate JSON concerns.

Recommended services:

- `AppSettingsStore` for settings.
- `HistoryStore` for reading progress and recent books.

Persistence should tolerate missing files and malformed JSON without crashing.

## External resources

External C handles must be wrapped in RAII objects.

For libzip:

- Archive handles must be closed or discarded safely.
- Entry handles must be closed safely.
- Error paths must not leak handles.

For libarchive:

- Archive handles must be freed safely.
- Error paths must not leak handles.
