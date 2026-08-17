# UI Behavior Specification

## Startup

On startup:

- Open the main window.
- Display application UI labels in Japanese.
- Do not automatically open the last book.
- Show the configured home folder in the sidebar.
- Keep the main image area empty until the user opens a folder, image file, or supported archive file.
- If the application is launched with a supported folder, image file, or archive file path, open that path after the main
  window is shown.

If no home folder is configured, use the user's home directory as the sidebar home folder.

## Sidebar

The sidebar is included in MVP and acts as a file browser.

It allows opening:

- Directories.
- Supported image files.
- Archive files.

When an archive book is open:

- The sidebar shows the archive file's parent folder.
- The current archive file is highlighted or selected.

### Sidebar display

- Hidden by default.
- Moving the mouse to the left edge shows the sidebar.
- Sidebar overlays the image area.
- Dragging the sidebar's right edge resizes the sidebar.

The sidebar width is persisted in settings and restored on startup. No settings UI is required.

### Sidebar controls

The sidebar must include:

- Current folder path.
- Home button.
- Back button.
- Forward button.
- Up directory button.
- Reload button.
- History button.
- Settings button.
- Sort button.
- File list.

### File list entries

The file list must show:

- Directories.
- Supported image files.
- Supported archive files.

The sort button sits below the navigation/settings buttons and opens a menu for changing file list sort order. It shows
the current direction and key, for example `↑ ファイル名`. Available sort keys are filename, created date, and modified
date, each with ascending and descending order.

Each applicable entry should show a reading state icon:

- Unread.
- Reading.
- Completed.

Directories with folder-book history use that folder book's reading state.

The file type icon is shown to the left of the file or folder name. Reading and completed state icons are shown at the
right edge of the row, with the state icon taking priority over long names when space is constrained.

Hovering over the same file list or history entry row long enough for a tooltip shows the full name near the mouse
cursor.

### File list behavior

Clicking a directory:

- Opens that directory as a folder book.
- Shows supported image files in that directory as pages.
- Ignores non-image files in that directory.
- Highlights the directory in the sidebar while that folder book is open, when the directory is visible in the current
  sidebar folder.

Double-clicking a directory changes the sidebar current directory.

Clicking a supported image file:

- Opens the parent folder as a folder book.
- Sets `currentPageIndex` to the clicked image.

Clicking a ZIP/CBZ or 7z/CB7 file:

- Opens the archive as an archive book.
- Restores `lastPageIndex` if history exists.
- Otherwise sets `currentPageIndex` to 0.
- Highlights the archive file in the sidebar.

Adjacent book navigation opens the previous or next book in the current book's parent folder. Adjacent book candidates
are folder books and supported archive books. Supported image files are pages within a folder book and are not adjacent
book candidates. Candidate order follows the sidebar file list order for that parent folder, with directories before
files and the configured sidebar sort key/order applied within each group. Navigation at the first or last candidate
does not wrap.

Reload refreshes the current sidebar folder contents. If the currently open book is that folder, reload also refreshes
the folder book's image pages and keeps the current image selected when the image still exists.

### Sidebar navigation

Home navigates to the configured home folder.

Back navigates to the previous sidebar folder if available.

Forward navigates to the next sidebar folder if available.

Up navigates to the parent directory.

After Up, Back, or Forward navigation, if the newly displayed file list contains the folder that navigation came from,
that folder is selected and scrolled into view. When the currently open folder book or archive is visible in the file
list, its selected entry is also scrolled into view.

History replaces the file list with recent books from reading history. History entries show the book name, path, reading
progress, reading state, and may show an asynchronously loaded thumbnail of the book's first page. Thumbnail loading may
be limited to the most recent entries to avoid excessive disk and CPU work.

Right-clicking the file list or history list opens a context menu for history-only operations. The menu can delete the
history entry for the clicked book, and can delete all history entries that belong to the current sidebar folder.

Settings replaces the file list with controls for editable app settings. Changes are persisted to `settings.json`.

## Header overlay

Header is included in MVP.

Display behavior:

- Hidden by default.
- Moving the mouse to the top edge shows the header.
- Header overlays the image area.
- The main window is frameless; the header provides the app's title-bar controls while visible.
- Dragging the header moves the window.
- Double-clicking the header toggles maximized/restored state.
- The close button's hit target reaches the top-right corner while the header is visible.

Required contents:

- Current opened book path.
- Single-page/spread toggle button.
- Right-to-left/left-to-right toggle button.
- Minimize, maximize/restore, and close buttons.

Settings button is not required in the header.

## Footer overlay

Footer is included in MVP.

Display behavior:

- Hidden by default.
- Moving the mouse to the bottom edge shows the footer.
- Footer overlays the image area.

Required contents:

- Current page number / total page count.
- Page slider.

### Slider direction

Left-to-right:

- Left side is the first page.
- Right side is the last page.

Right-to-left:

- Right side is the first page.
- Left side is the last page.

## Overlay behavior

MVP uses fixed overlay behavior:

- Edge trigger size: 24 px.
- Hide delay: 800 ms.

Header shows when the mouse is within 24 px of the top edge.

Footer shows when the mouse is within 24 px of the bottom edge.

Sidebar shows when the mouse is within 24 px of the left edge.

Each overlay hides 800 ms after the mouse leaves its active area.

The frameless main window remains resizable from its outer edges when it is not maximized.

## File operations

MVP must not modify user book files.

Not included:

- Delete file.
- Rename file.
- Move file.
- Edit archive contents.
- Extract archive contents through UI.

The app is read-only with respect to user books and image files.
