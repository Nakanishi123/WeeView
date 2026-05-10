# WeeView Implementation Plan

This plan is ordered to keep Codex tasks small and reviewable.

Before each non-trivial change, read:

- [`spec.md`](./spec.md) for the specification entry point.
- The relevant focused spec under `docs/specs/`.
- [`architecture.md`](./architecture.md) for structure and ownership rules.

## Step 1: Project skeleton

Goal:

- Create a buildable C++23 Qt Widgets application.

Tasks:

- Create CMake project.
- Add `src/main.cpp`.
- Add `MainWindow`.
- Show an empty central viewer.
- Confirm build and app launch.

Acceptance:

- App opens a main window.
- No QML or `.ui` files are used.

## Step 2: Core enums and models

Goal:

- Add basic shared data types.

Tasks:

- Add `ViewMode`.
- Add `ReadingDirection`.
- Add `BookType`.
- Add `PageInfo`.
- Add `ViewerState`.
- Add `HistoryEntry`.

Acceptance:

- Types compile.
- `ViewerState` does not contain `spreadAnchorPage`.

## Step 3: Utility functions

Goal:

- Implement file type detection and natural sorting.

Tasks:

- Add `FileTypes` helpers.
- Add `NaturalSort`.
- Add tests if a test framework exists.

Acceptance:

- `1.jpg`, `2.jpg`, `10.jpg` sort in that order.
- Supported image/archive extensions are detected case-insensitively.

## Step 4: JSON settings/history stores

Goal:

- Persist settings and history with JSON.

Tasks:

- Add `AppSettings` model.
- Add `AppSettingsStore`.
- Add `HistoryStore`.
- Use `QStandardPaths::AppConfigLocation`.
- Use `QSaveFile` for saves.

Acceptance:

- Missing files produce defaults.
- Malformed JSON does not crash the app.
- Saving writes valid JSON.
- `history.json` does not contain `spreadAnchorPage`.

## Step 5: FolderBook

Goal:

- Open a folder as a book.

Tasks:

- Add `Book` interface.
- Add `FolderBook`.
- Scan supported image files.
- Ignore unsupported files.
- Apply natural sort.
- Load pages as `QImage`.

Acceptance:

- Opening a folder displays the correct page count.
- Page order is natural sorted.

## Step 6: MangaView single-page display

Goal:

- Display one page centered and fit to available area.

Tasks:

- Add `MangaView`.
- Render `QImage` in single-page mode.
- Preserve aspect ratio.
- Refit on resize.

Acceptance:

- Image is centered.
- Image fits within the view.
- No zoom or pan UI is required.

## Step 7: Page navigation

Goal:

- Add keyboard and mouse page navigation.

Tasks:

- Implement next/previous page.
- Implement Home/End.
- Implement reading-direction-aware Left/Right keys.
- Implement mouse button navigation.
- Implement wheel up/down navigation.

Acceptance:

- Right-to-left: left click/Left Arrow means next page.
- Left-to-right: right click/Right Arrow means next page.
- Space and PageDown always mean next page.
- Wheel down always means next page.

## Step 8: Header and footer overlays

Goal:

- Add MVP overlay controls.

Tasks:

- Add `OverlayContainer`.
- Add `HeaderBar`.
- Add `FooterBar`.
- Add current book path display.
- Add view mode toggle.
- Add reading direction toggle.
- Add page counter.
- Add page slider.
- Implement edge-trigger show/hide.

Acceptance:

- Top edge shows header.
- Bottom edge shows footer.
- Footer slider direction changes with reading direction.

## Step 9: Spread mode

Goal:

- Add single/spread switching and spread rendering.

Tasks:

- Implement spread group calculation based on `currentPageIndex`.
- Render two pages when possible.
- Render one page for landscape pages.
- Apply reading-direction visual order.
- Add next/previous movement by display group.

Acceptance:

- Enabling spread on page 1 shows logical `[1, 2]`.
- Enabling spread on page 2 shows logical `[2, 3]`.
- Right-to-left visual order for `[1, 2]` is `[2][1]`.
- Left-to-right visual order for `[1, 2]` is `[1][2]`.

## Step 10: ZIP/CBZ support

Goal:

- Open ZIP/CBZ as books.

Tasks:

- Add libzip dependency.
- Add RAII wrappers for libzip handles.
- Add `ArchiveReader`.
- Add `ZipArchiveReader`.
- Add `ZipBook`.
- Sort internal entries by full path using natural sort.

Acceptance:

- `.zip` and `.cbz` files open.
- Internal image entries are displayed in natural order.
- Unsupported internal files are ignored.

## Step 11: Sidebar

Goal:

- Add MVP file browser.

Tasks:

- Add `Sidebar`.
- Show current folder path.
- Add Home/Back/Forward/Up/Reload controls.
- Add file list.
- Show directories, supported image files, and ZIP/CBZ files.
- Click directories to navigate.
- Click image files to open parent folder and jump to clicked image.
- Click ZIP/CBZ files to open archive.
- Highlight current ZIP/CBZ when an archive is open.

Acceptance:

- Startup shows configured home folder or user home directory.
- Sidebar opens from the left edge.
- Current archive's parent folder is shown when ZIP/CBZ is open.

## Step 12: History integration

Goal:

- Save and restore reading progress.

Tasks:

- Update history on book close/change/page change.
- Save `lastPageIndex`, `lastDisplayLastPageIndex`, `spreadGroupDirection`, `viewMode`, `readingDirection`, `pageCount`, and timestamp.
- Derive sidebar reading state.

Acceptance:

- Reopening a ZIP/CBZ restores `lastPageIndex` when history exists.
- `lastPageIndex` is the logical first page index of the displayed group.
- Sidebar shows unread/reading/completed state.

## Step 13: Image cache

Goal:

- Add basic decoded image caching.

Tasks:

- Implement `ImageCache`.
- Cache current, previous 2, and next 4 pages.
- Limit cache to 8 pages.
- Clear cache when changing books.

Acceptance:

- Page transitions reuse cached `QImage` where available.
- Cache never exceeds 8 pages.

## Step 14: AVIF hardening

Goal:

- Ensure AVIF support works on target environments.

Tasks:

- Verify AVIF through `QImageReader`.
- Keep decoder modular.
- Add fallback backend only if required.

Acceptance:

- AVIF files open in supported deployment environments.
- Decoder changes do not alter viewer behavior.
