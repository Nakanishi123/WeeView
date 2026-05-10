# UI Behavior Specification

## Startup

On startup:

- Open the main window.
- Do not automatically open the last book.
- Show the configured home folder in the sidebar.
- Keep the main image area empty until the user opens a folder, image file, ZIP, or CBZ.

If no home folder is configured, use the user's home directory as the sidebar home folder.

## Sidebar

The sidebar is included in MVP and acts as a file browser.

It allows opening:

- Directories.
- Supported image files.
- ZIP/CBZ files.

When a ZIP/CBZ book is open:

- The sidebar shows the ZIP/CBZ file's parent folder.
- The current ZIP/CBZ file is highlighted or selected.

### Sidebar display

- Hidden by default.
- Moving the mouse to the left edge shows the sidebar.
- Sidebar overlays the image area.

MVP uses fixed overlay behavior. No settings UI is required.

### Sidebar controls

The sidebar must include:

- Current folder path.
- Home button.
- Back button.
- Forward button.
- Up directory button.
- Reload button.
- File list.

### File list entries

The file list must show:

- Directories.
- Supported image files.
- Supported ZIP/CBZ files.

Each applicable entry should show a reading state icon:

- Unread.
- Reading.
- Completed.

### File list behavior

Clicking a directory changes the sidebar current directory.

Clicking a supported image file:

- Opens the parent folder as a folder book.
- Sets `currentPageIndex` to the clicked image.

Clicking a ZIP/CBZ file:

- Opens the archive as a ZIP book.
- Restores `lastPageIndex` if history exists.
- Otherwise sets `currentPageIndex` to 0.
- Highlights the ZIP/CBZ file in the sidebar.

### Sidebar navigation

Home navigates to the configured home folder.

Back navigates to the previous sidebar folder if available.

Forward navigates to the next sidebar folder if available.

Up navigates to the parent directory.

Reload rescans the current directory.

## Header overlay

Header is included in MVP.

Display behavior:

- Hidden by default.
- Moving the mouse to the top edge shows the header.
- Header overlays the image area.

Required contents:

- Current opened book path.
- Single-page/spread toggle button.
- Right-to-left/left-to-right toggle button.

Settings button is not required in MVP.

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

## File operations

MVP must not modify user book files.

Not included:

- Delete file.
- Rename file.
- Move file.
- Edit archive contents.
- Extract archive contents through UI.

The app is read-only with respect to user books and image files.
