---
vibedoc: 1
id: ARCH-0001
kind: architecture
status: accepted
tags:
  - architecture
  - modules
  - gpui
related:
  - RES-0002
  - ARCH-0002
  - ARCH-0003
  - ARCH-0004
---

# System overview

## 方針

Rust/GPUI版はQt版のClass構成を再現しない。Domain rule、非同期調整、I/O、GPUI表示を分離し、見開きなどの重要規則をUI frameworkなしでTestできる構成にする。

## Module

```text
src/
├── main.rs
├── domain/
├── application/
├── book/
├── archive/
├── image/
├── persistence/
├── filesystem/
└── ui/
```

### `domain`

GPUI、SQLite、File systemへ依存しない型と純粋な状態遷移を置く。

- Page/BookのNewtypeとEnum。
- Display group計算。
- Viewer navigation。
- Reading state判定。
- History復元候補。
- Sort設定などのDomain value。

### `application`

利用者ActionとBackground処理を調整する。

- Book openと切替。
- Page request generation。
- Metadata/Decode priorityとdebounce。
- Cache preload計画。
- History更新。
- Rename workflow。
- File watcher event処理。

### `book`

Folder本とArchive本のページ列を提供する。最初からTrait object階層を作らず、閉じた種類はEnumで表現する。

```rust
enum BookSource {
    Folder(FolderBook),
    Archive(ArchiveBook),
}
```

### `archive`

ZIPと7zのEntry列挙・読み出しを閉じ込める。

```rust
enum ArchiveSource {
    Zip(ZipArchive),
    SevenZip(SevenZipArchive),
}
```

7z専用の高度な最適化を加えない。

### `image`

Metadata、Decode、描画形式変換、Decoded image cache、GPUI `RenderImage` の寿命を扱う。標準 `img(path)` にCache policyを任せない。

### `persistence`

SQLite migration、Settings、Folder sort、History、Legacy JSON importを扱う。UIからSQLを直接実行しない。

### `filesystem`

File列挙、Natural sort、Rename、Path validation、Symlink、Watcherを扱う。

### `ui`

GPUI固有のWindow、Viewer element、Header、Footer、Sidebar、Toast、Dialog、入力Mappingを置く。Render中にDecode、Archive走査、SQLを実行しない。

## 依存方向

```text
ui ───────────────┐
                  v
application ──> domain
    │             ^
    ├─> book ─────┤
    │     └─> archive
    ├─> image ────┤
    ├─> persistence
    └─> filesystem
```

外側のModuleはDomainへ依存できるが、Domainは外側へ依存しない。I/O Module同士を直接結びすぎず、Rename後のDB更新など複数境界にまたがる処理はApplicationが順序を管理する。

## Ownership

- Exclusive ownershipを既定にする。
- `Arc`、`Mutex`、`Box`、`dyn Trait` は必要な共有・Framework境界がある場合だけ使う。
- GPUIが要求する `Arc<RenderImage>` のような共有は意図を文書化する。
- Domain stateへUI handleやSQLite connectionを入れない。
- Cache entryが描画画像の寿命を所有し、不要な二重Pixel bufferを長期保持しない。

## Error

I/O固有Errorを境界で型付きApplication errorへ変換する。Errorを `.ok()` や空Matchで捨てず、利用者表示、再試行、Log、無視可能な既知状態のいずれかを明示する。
