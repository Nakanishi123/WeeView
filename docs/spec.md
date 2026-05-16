# WeeView MVP Specification

This is the entry point for WeeView's documented behavior.

Keep this file short. Detailed behavior is split into focused documents:

- [Input and sorting](./specs/input-and-sorting.md)
- [Viewer behavior](./specs/viewer-behavior.md)
- [UI behavior](./specs/ui-behavior.md)
- [Settings and history](./specs/settings-history.md)

Implementation structure is documented in [architecture.md](./architecture.md).  
Implementation order is documented in [implementation-plan.md](./implementation-plan.md).  
Project decisions and deferred work are documented in [decisions.md](./decisions.md).

## Product summary

WeeView is a cross-platform manga/image viewer for Windows and Linux.

MVP goals:

- Open image folders.
- Open ZIP/CBZ and 7z/CB7 archives.
- Display images as pages.
- Support single-page and spread-page viewing.
- Support right-to-left and left-to-right reading.
- Provide a file-browser sidebar.
- Keep the image area unobstructed with overlay UI.

NeeView is an inspiration, but WeeView does not need to match all NeeView behavior.

## Technology requirements

Use:

- C++23
- Qt 6 Widgets
- CMake
- libzip for ZIP/CBZ
- libarchive for 7z/CB7
- JSON files for settings and history

Do not use:

- QML
- Qt Designer `.ui` files
- Electron

All UI must be created programmatically in C++.

## MVP scope

Included:

- Folder books.
- ZIP/CBZ and 7z/CB7 archive books.
- JPEG, PNG, WebP, and AVIF images.
- Natural sorting.
- Single-page view.
- Spread view.
- Right-to-left and left-to-right reading direction.
- Direction-aware keyboard and mouse navigation.
- Sidebar file browser.
- Header and footer overlays.
- Basic decoded image cache.
- JSON settings.
- JSON history and reading progress.
- Sidebar settings panel.

Excluded:

- RAR/CBR.
- Manual zoom.
- Pan.
- Bookmarks.
- File delete/rename/move.
- Archive editing or extraction UI.

## Core behavioral invariants

- Page indices are zero-based internally.
- `currentPageIndex` means the logical first page of the current display group.
- `lastPageIndex` in history has the same meaning as `currentPageIndex`.
- History also stores the current display group's logical last page and spread group direction so spread groups restore exactly.
- Do not persist `spreadAnchorPage`.
- Right-to-left is the default reading direction.
- Single-page mode is the default view mode.
- AVIF is required for MVP.
- User book files must be treated as read-only.
