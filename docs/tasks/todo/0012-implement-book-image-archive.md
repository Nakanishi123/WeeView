---
vibedoc: 1
id: TASK-0012
kind: task
status: todo
tags:
  - book
  - image
  - archive
  - cache
related:
  - DEC-0001
  - ARCH-0003
depends_on:
  - TASK-0010
  - TASK-0011
---

# Book・Image・Archive層を実装する

## Scope

- Folder本、ZIP/CBZ、単純な7z/CB7。
- Page IDとNatural sort。
- Metadata priority、Background decode、Generation。
- JPEG、PNG、WebP、AVIF。
- 512 MiB decoded cacheと128 MiB Archive byte cache。
- 個別Page errorと再試行。
- EXIF Orientationは負荷が大きければ別Taskへ分離する。

## Acceptance

- 対応形式を同じDecode経路で扱える。
- UI threadでArchive走査またはDecodeしない。
- 7z専用の高度な最適化を導入しない。
- Current groupをCacheからEvictしない。
- 壊れたPageがPage番号をずらさない。

