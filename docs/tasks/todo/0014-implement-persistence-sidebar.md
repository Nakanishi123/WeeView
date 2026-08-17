---
vibedoc: 1
id: TASK-0014
kind: task
status: todo
tags:
  - sqlite
  - sidebar
  - history
  - settings
related:
  - DEC-0004
  - DEC-0005
  - ARCH-0004
depends_on:
  - TASK-0012
  - TASK-0013
---

# SQLite・Sidebar・History・Settingsを実装する

## Scope

- SQLite Schema、Migration、WAL、Busy retry。
- Qt JSON import。
- Settings、Folder sort、History。
- Files/History/Settings View。
- Directory Single/Double click判定。
- 前の本／次の本。
- Rename workflow。
- Thumbnailは別Taskへ分離してよい。

## Acceptance

- 複数Processから異なる本を更新してもHistoryを失わない。
- 同じ本は最終更新が勝つ。
- Page ID優先、Index fallbackで復元する。
- Rename後に関連PathをComponent単位で更新する。
- DB失敗時にFile RenameをRollbackしない。
- Reset settingsでHistoryを削除しない。

