# Input and Sorting Specification

## Supported input sources

WeeView can open supported paths passed by the operating system, such as a file manager's "Open With" action.

When multiple paths are passed at startup, WeeView opens the first supported path.

### Folder books

A folder containing supported image files can be opened as a book.

Supported image extensions:

- `.jpg`
- `.jpeg`
- `.png`
- `.webp`
- `.avif`

When a supported image file is opened from the sidebar:

1. Open its parent folder as a folder book.
2. Set the current page to the clicked image.

If a folder contains both images and archive files:

- Images are pages of the folder book.
- Archive files remain visible in the sidebar.
- Clicking an archive file opens it as a separate book.

### Archive books

Supported archive extensions:

- `.zip`
- `.cbz`
- `.7z`
- `.cb7`

Unsupported in MVP:

- `.rar`
- `.cbr`

When an archive book is open:

- The sidebar shows the archive's parent folder.
- The current archive file is highlighted or selected.

## Supported image formats

MVP must support:

- JPEG
- PNG
- WebP
- AVIF

AVIF is required.

The decoder should first use Qt image loading. If AVIF is not available through Qt image plugins on a target environment, the decoder layer must allow adding a dedicated AVIF backend without changing viewer behavior.

## Ignored files

When building page lists, ignore:

- Directories.
- Unsupported image files.
- `.DS_Store`.
- Files under `__MACOSX/`.

Hidden file handling may become configurable later. MVP may ignore hidden files.

## Natural sorting

All page lists and sidebar file lists must use natural sorting.

Expected order:

1. `1.jpg`
2. `2.jpg`
3. `10.jpg`

Japanese kanji numerals in names are also treated as numeric tokens.

Expected order:

1. `第一景.jpg`
2. `第二景.jpg`
3. `第十景.jpg`

Natural sorting applies by default to:

- Folder image files.
- Archive internal image entries.

The sidebar file list defaults to natural filename sorting, and can also be sorted by filename, created date, or modified
date in either ascending or descending order.

When a folder book is opened, its image page order follows the sidebar sort setting for that folder. Changing the
folder's sidebar sort order while that folder book is open refreshes the book page order and keeps the current image
selected when the image still exists.

For archives, sort by the full internal archive path.

ZIP filename decoding should prefer UTF-8. Non-UTF-8 filenames may be treated as a known MVP limitation.

7z filename decoding should prefer UTF-8.
