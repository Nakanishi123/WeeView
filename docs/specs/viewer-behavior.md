# Viewer Behavior Specification

## Core concepts

A page is one image.

A page is landscape when `width > height`.

Landscape pages are displayed as single pages in spread mode.

Spread grouping is based on page metadata, not decoded image cache state.

Page metadata may be loaded lazily. The app should prioritize metadata for the current display group before painting and
may continue loading metadata for nearby and remaining pages after the book is open.

`currentPageIndex` is zero-based and means the logical first page index of the current display group.

- In single-page mode, it is the displayed page.
- In spread mode, it is the logical first page index of the current directional local group.
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

Mouse clicks navigate by button, regardless of click position.

Right-to-left:

- Left click: next page.
- Right click: previous page.

Left-to-right:

- Left click: previous page.
- Right click: next page.

Mouse wheel navigation is independent of reading direction:

- Wheel down: next page.
- Wheel up: previous page.

Right-button hold gestures navigate by horizontal command shape, independent of reading direction:

- Right then left: advance by one logical page.
- Left then right: go back by one logical page.

In spread mode, right-button hold gestures still move by exactly one logical page. A one-page advance from a single
portrait page first pairs it with the next portrait page when possible, such as `[1]` to `[1, 2]`, instead of skipping
to `[2, 3]`.

While the right button is held, the viewer shows a centered translucent black command watermark with translucent white
text for the active gesture command.

Mouse wheel zoom is not part of MVP.

## Page loading during rapid navigation

Page state changes immediately during slider, keyboard, mouse click, and mouse wheel navigation.

Image loading after page navigation may be delayed by `pageLoadDebounceMs`.

While delayed loading is pending:

- The most recent successfully painted page or spread remains visible if the new target page is not loaded yet.
- A translucent centered page indicator shows the page or spread that will be displayed after loading.
- The final target page is loaded after navigation has been quiet for `pageLoadDebounceMs`.
- Logical page grouping continues to use page metadata and must not shift because target images are not loaded yet.

If `pageLoadDebounceMs <= 0`, page images load immediately after each page navigation.

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

Spread mode uses directional local grouping.

Do not use fixed pairs like `[1]`, `[2, 3]`, `[4, 5]`.

Do not use spread anchor parity.

A display group is either:

- A single page: `[N]`.
- A spread: `[N, N + 1]`.

Landscape pages are single-page groups.

When switching from single-page mode to spread mode:

- Keep `currentPageIndex` unchanged as `N`.
- Create a forward group starting at `N`.
- Switch view mode to spread.

Examples use 1-based page numbers:

- Enabling spread on page 1 displays `[1, 2]`.
- Enabling spread on page 2 displays `[2, 3]`.
- Enabling spread on page 3 when page 4 is landscape displays `[3]`.
- Enabling spread on page 4 when page 4 is landscape displays `[4]`.

Forward group from `N`:

- If `N` is landscape, display `[N]`.
- Else if `N + 1` does not exist, display `[N]`.
- Else if `N + 1` is landscape, display `[N]`.
- Else display `[N, N + 1]`.

## Spread navigation

In spread mode, next/previous navigation moves by directional local display groups.

Next:

- Let `N = currentGroup.last + 1`.
- Create a forward group starting at `N`.

Previous:

- Let `N = currentGroup.first - 1`.
- Create a backward group ending at `N`.
- If `N` is before the first page, do not move.

Backward group ending at `N`:

- If `N` is landscape, display `[N]`.
- Else if `N - 1` does not exist, display `[N]`.
- Else if `N - 1` is landscape, display `[N]`.
- Else display `[N - 1, N]`.

When the current group already includes the final page, next navigation does not move.

Boundary examples:

- Previous from `[2, 3]` displays `[1]`.
- Next from `[99, 100]` does not move when page 100 is the final page.

## Landscape pages in spread mode

A landscape page is displayed alone.

If a forward or backward group would pair with a landscape page, display the non-landscape page alone.

Forward and backward navigation are not symmetrical around landscape pages.

The app does not remember the previous forward path. It recalculates the previous group from the page immediately before the current group.

Example pages:

- Page 1: portrait.
- Page 2: portrait.
- Page 3: portrait.
- Page 4: landscape.
- Page 5: portrait.
- Page 6: portrait.

Forward from `[1, 2]`:

- `[1, 2]`.
- Next: `[3]`.
- Next: `[4]`.
- Next: `[5, 6]`.

Backward from `[5, 6]`:

- `[5, 6]`.
- Previous: `[4]`.
- Previous: `[2, 3]`.

This asymmetry is expected.

## Spread visual order

Logical grouping is independent from visual order.

If metadata for a future page has not been loaded yet, that page may be treated as portrait until its metadata is
available. When metadata is loaded, the current display group should be recalculated from the same viewer state.

Right-to-left:

- The higher/next page is shown on the left.
- The lower/current page is shown on the right.

Left-to-right:

- The lower/current page is shown on the left.
- The higher/next page is shown on the right.

In spread mode, each page fits within its half of the view, but the two pages are aligned to the center seam:

- The left visual page's right edge touches the center seam.
- The right visual page's left edge touches the center seam.
- Vertical alignment remains centered.

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
- Additional pages after the current display group use about 2/3 of the remaining cache memory.
- Additional pages before the current display group use about 1/3 of the remaining cache memory.
- If one side cannot use its share, the unused memory may be used by the other side.

Maximum cache size is configured by `imageCacheMemoryLimitMiB`.

The current display group is protected from eviction. If the current page or spread exceeds the configured cache limit,
it remains displayable and preloading is reduced or skipped.
