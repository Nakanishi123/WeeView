---
vibedoc: 1
id: TASK-0015
kind: task
status: todo
tags:
  - watcher
  - thumbnail
  - polish
related:
  - DEC-0004
  - DEC-0006
  - ARCH-0004
depends_on:
  - TASK-0014
---

# File watcherと仕上げ機能を追加する

## Scope

- 現在Folder本、Archive、Sidebar folder、Home folderの監視。
- Event debounceとArchive安定待ち。
- History thumbnailの非同期・可視範囲優先Loading。
- Tooltip、細部のPlatform UX、EXIF Orientation未実装時の対応。

## Acceptance

- 外部変更後もPage IDで現在画像を維持する。
- Archive再Open失敗時に最後の正常表示を維持する。
- 全History pathを監視しない。
- ThumbnailをSQLite BLOBへ保存しない。
- Watcher未実装状態で動かないSettings toggleを表示しない。

