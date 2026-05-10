# WeeView Decisions and Deferred Work

This document records choices made for the MVP and work intentionally deferred.

## 1. Accepted MVP decisions

### C++ version

Use C++23.

### GUI framework

Use Qt 6 Widgets.

Do not use:

- QML
- Qt Designer `.ui` files
- Electron

### Persistence

Use JSON files.

Do not use QSettings for MVP persistence.

### Archive support

Use libzip.

MVP officially supports:

- ZIP
- CBZ

Deferred:

- RAR
- CBR
- 7z
- CB7

### Image formats

MVP requires:

- JPEG
- PNG
- WebP
- AVIF

### Default reading direction

Default to right-to-left.

### Sorting

Use natural sorting everywhere page/file order is visible.

### Spread history

Do not persist `spreadAnchorPage`.

Persist `lastPageIndex`, `lastDisplayLastPageIndex`, and `spreadGroupDirection` so directional local spread groups can be restored exactly.

### Zoom

Zoom is not part of MVP.

### Bookmarks

Bookmarks are not part of MVP.

### File modification

The MVP is read-only with respect to user files.

Do not implement:

- Delete
- Rename
- Move
- Archive editing
- Extraction UI

## 2. Known MVP limitations

### Landscape pages in spread mode

Landscape pages are displayed as single pages.

Pairing around landscape pages may shift when navigating backward or forward.

This is accepted behavior because spread mode uses directional local grouping. The app recalculates the previous group from the page immediately before the current group and does not remember the previous forward path.

Do not replace this with fixed pair generation or spread anchor parity.

### ZIP filename encoding

UTF-8 is preferred.

Non-UTF-8 ZIP filenames may be a known limitation in MVP.

### Cache policy

MVP cache is page-count based.

Future cache policy may be memory-size based.

## 3. Deferred features

Potential post-MVP features:

- Manual zoom.
- Ctrl + wheel zoom.
- Fit width.
- Fit height.
- Panning.
- Bookmark support.
- Settings UI.
- Configurable overlay behavior.
- Configurable landscape threshold.
- RAR/CBR support.
- 7z/CB7 support.
- Memory-size based cache eviction.
- Keybinding customization.
- File operations such as delete/rename/move.
- More robust ZIP filename encoding handling.

## 4. Conflict policy

If code and docs conflict:

1. Report the conflict.
2. Do not silently change documented behavior.
3. Ask for direction, unless the user already gave an explicit instruction.
4. Update docs and tests when behavior changes.
