# Viewer Behavior Specification

## Core concepts

A page is one image.

A page is landscape when `width > height`.

Landscape pages are displayed as single pages in spread mode.

`currentPageIndex` is zero-based and means the logical first page index of the current display group.

- In single-page mode, it is the displayed page.
- In spread mode, it is the lower page index of the preferred spread pair.
- If the display group is a single page because of an edge case or landscape page, it is that page.

`lastPageIndex` in history uses the same definition.

## View modes

Supported modes:

- Single page.
- Spread.

Default mode: single page.

## Reading direction

Supported directions:

- Right-to-left.
- Left-to-right.

Default direction: right-to-left.

Reading direction affects:

- Left/right keyboard behavior.
- Mouse click behavior.
- Spread visual page order.
- Footer slider direction.

## Keyboard navigation

Right-to-left:

- Left Arrow: next page.
- Right Arrow: previous page.

Left-to-right:

- Right Arrow: next page.
- Left Arrow: previous page.

Direction-independent keys:

- Space: next page.
- Backspace: previous page.
- PageDown: next page.
- PageUp: previous page.
- Home: first page.
- End: last page.

## Mouse navigation

Left-clicking the image area moves toward the clicked side.

Right-to-left:

- Click left half: next page.
- Click right half: previous page.

Left-to-right:

- Click right half: next page.
- Click left half: previous page.

Mouse wheel zoom is not part of MVP.

## Single-page mode

Single-page mode displays exactly one page.

Behavior:

- Display `currentPageIndex`.
- Center the image.
- Fit the image to the available page area.
- Preserve aspect ratio.
- Refit when the window size changes.

## Spread mode

Spread mode displays one or two pages.

Spread mode uses a NeeView-style anchor behavior represented by `currentPageIndex`.

When switching from single-page mode to spread mode:

- Keep `currentPageIndex` unchanged.
- Switch view mode to spread.

Examples use 1-based page numbers:

- Enabling spread on page 1 displays `[1, 2]`.
- Enabling spread on page 2 displays `[2, 3]`.
- Enabling spread on page 3 displays `[3, 4]`.

For a logical current page `N`, the preferred spread group is `[N, N + 1]`.

If `N + 1` does not exist, display `[N]`.

## Spread navigation

In spread mode, next/previous navigation moves by display groups.

For normal portrait pages:

- Next page increases `currentPageIndex` by 2.
- Previous page decreases `currentPageIndex` by 2.

At boundaries, clamp to a valid page index.

If the target group would include a landscape page, apply the landscape page rules.

## Landscape pages in spread mode

A landscape page is displayed alone.

If a preferred spread group contains a landscape page, collapse according to NeeView-compatible behavior.

MVP accepts that pairing around landscape pages may shift when navigating backward or forward. This is known MVP behavior, not a bug unless this document later changes.

## Spread visual order

Logical grouping is independent from visual order.

Right-to-left:

- The higher/next page is shown on the left.
- The lower/current page is shown on the right.

Left-to-right:

- The lower/current page is shown on the left.
- The higher/next page is shown on the right.

## Zoom

Zoom is not part of MVP.

MVP behavior:

- Fit image to available page area.
- Preserve aspect ratio.
- Center image.
- Refit on window resize.

Not included:

- Manual zoom.
- Ctrl + wheel zoom.
- Fit width.
- Fit height.
- Panning.

## Cache

MVP includes basic decoded image caching.

Cache decoded images, not UI pixmaps.

Initial cache window:

- Current page or pages.
- Previous 2 pages.
- Next 4 pages.

Maximum cache size: 8 pages.

The MVP cache is page-count based, not memory-size based.
