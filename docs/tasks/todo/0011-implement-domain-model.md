---
vibedoc: 1
id: TASK-0011
kind: task
status: todo
tags:
  - domain
  - viewer
related:
  - DEC-0002
  - DEC-0003
  - ARCH-0002
depends_on:
  - TASK-0010
---

# Viewer Domain modelを実装する

## Scope

- Page/BookのNewtypeとEnum。
- Display group、Viewer position、Reading state。
- Forward/Backward grouping。
- Next、Previous、Home、End、1 logical page step。
- History復元規則。
- Qt版の横長ページ周辺、Home、Endを含むTable-driven test。

## Acceptance

- Domain moduleがGPUI、SQLite、File systemへ依存しない。
- Directional local groupingの非対称例をTestする。
- `End` がSpreadでも最終ページ単独になる。
- 読了判定がGroup末尾を使う。
- 生のBoolean flagでModeやDirectionを表現しない。

