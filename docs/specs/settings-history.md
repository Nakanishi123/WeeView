# Settings and History Specification

## Storage format

Settings and history must be stored as JSON files.

Use safe writes for JSON saves. Prefer atomic save behavior rather than directly overwriting files.

Recommended storage location:

- Qt app config location.

## Settings

Recommended file name:

- `settings.json`

Required settings:

- Schema version.
- Home folder.
- Default reading direction.
- Default view mode.
- Overlay edge trigger size.
- Overlay hide delay.
- Page load debounce duration.
- Sidebar width.
- Window width and height.
- Window maximized state.

If no home folder is configured, use the user's home directory.

Default values:

- Reading direction: right-to-left.
- View mode: single page.
- Overlay edge trigger size: 24 px.
- Overlay hide delay: 800 ms.
- Page load debounce duration: 120 ms.
- Sidebar width: 320 px.
- Window size: 960 x 720.
- Window maximized state: false.

`pageLoadDebounceMs` configures the page image load delay after page navigation.

- If `pageLoadDebounceMs > 0`, page image loading after page navigation waits for that many milliseconds without another page change.
- If `pageLoadDebounceMs <= 0`, page image loading after page navigation happens immediately.

`imageCacheMemoryLimitMiB` configures the decoded image cache memory limit in MiB.

- Values less than 1 are treated as 1 MiB.
- The default is 256 MiB.
- The current display group may exceed this limit when a currently displayed image is larger than the limit.

On close, the app saves the current window maximized state and normal window size. If the app was closed while
maximized, the next startup restores maximized state and preserves the last normal window size for later non-maximized
use.

## History

Recommended file name:

- `history.json`

Bookmarks are not part of MVP.

History must store recent books and reading progress.

History is capped to the 200 most recently opened unique book paths. If duplicate entries exist for a book path, keep
the most recent entry.

Required history fields per book:

- Book path.
- Book type: folder or archive.
- Display name.
- Last page index: logical first page index of the displayed group.
- Last display last page index: logical last page index of the displayed group.
- Page count.
- View mode.
- Reading direction.
- Spread group direction: forward or backward.
- Last opened timestamp.

Do not persist `spreadAnchorPage`.

`lastPageIndex` has the same meaning as `currentPageIndex`: the logical first page index of the currently displayed group.

`lastDisplayLastPageIndex` and `spreadGroupDirection` are required to restore directional local spread groups. For example, `[1]` and `[1, 2]` both have `lastPageIndex` 0, so `lastPageIndex` alone is not enough to restore the display group.

## Reading state

The sidebar reading state is derived from history.

Unread:

- No history entry exists.

Reading:

- `lastPageIndex > 0`.
- `lastPageIndex < pageCount - 1`.

Completed:

- `lastPageIndex >= pageCount - 1`.
