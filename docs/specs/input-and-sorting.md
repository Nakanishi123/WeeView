# Input and Sorting Specification

## Supported input sources

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

If a folder contains both images and ZIP/CBZ files:

- Images are pages of the folder book.
- ZIP/CBZ files remain visible in the sidebar.
- Clicking a ZIP/CBZ opens it as a separate book.

### ZIP/CBZ books

Supported archive extensions:

- `.zip`
- `.cbz`

Unsupported in MVP:

- `.rar`
- `.cbr`
- `.7z`
- `.cb7`

When a ZIP/CBZ book is open:

- The sidebar shows the archive's parent folder.
- The current ZIP/CBZ file is highlighted or selected.

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

Natural sorting applies to:

- Folder image files.
- ZIP/CBZ internal image entries.
- Sidebar file list.

For ZIP/CBZ archives, sort by the full internal archive path.

ZIP filename decoding should prefer UTF-8. Non-UTF-8 filenames may be treated as a known MVP limitation.
