---
vibedoc: 1
id: DEC-0001
kind: decision
status: accepted
tags:
  - product-scope
  - migration
related:
  - RES-0001
  - TASK-0010
---

# Rust/GPUI版の製品範囲と開発段階

## 決定

WeeViewはWindowsとLinux向けの漫画・画像Viewerとする。Qt版の内部構造は移植せず、利用者から見える機能と議論して確定した挙動をRustとGPUIで再構築する。

## 対応する本

### Folder本

- Directory直下の対応画像をページとして扱う。
- 再帰的に子Directoryの画像を含めない。
- 画像Fileを開いた場合は親DirectoryをFolder本として開き、その画像へ移動する。

### Archive本

- ZIP、CBZ、7z、CB7を直接開く。
- 書庫内部の対応画像をフルEntry pathでNatural sortする。
- 利用者から見える場所へ展開しない。
- 7z/CB7は正しく非同期に読めることを優先し、高度なRandom access最適化を行わない。

## 対応画像

必須形式はJPEG、PNG、WebP、AVIFとする。AVIFは任意機能ではない。

ページ列挙では非対応形式、Directory、`.DS_Store`、`__MACOSX/` 以下を除外する。対応拡張子の画像がDecodeできない場合はページ列から除外せず、エラーページを表示する。

## 段階

### PoC

- 3000×5000画像のBackground decode。
- GPU描画。
- Fit Window。
- Pointer-centered zoom。
- Pan。
- Page switch、debounce、cache。
- Frameless WindowのPlatform検証。

### Core

- Folder、ZIP、7z。
- Single/Spread。
- RTL/LTR。
- Page navigation。
- 非同期Metadata/Decode。
- Cacheとエラー表示。

### Usable

- Sidebar。
- HistoryとSQLite。
- Settings。
- Header/Footer overlay。
- 前の本／次の本。
- Rename。
- 高速ページ送り。

### Polish

- Thumbnail。
- Mouse gesture。
- Tooltip。
- File watcher。
- EXIF Orientation。実装負荷が低ければ前倒ししてよい。

## Non-goal

- RAR、CBR。
- Fullscreen。Maximizeは対応する。
- Delete、Move、Archive編集、Archive内部Rename、画像変換。
- 7z専用のDisk cacheまたはSolid archive最適化。
- Zoom/Pan状態の保存。
- Qt版Architectureの再現。

## 結果

技術的不確実性が高いViewer描画をPoCで先に検証できる。製品仕様に含まれるDeferred機能は、未実装中に動かない設定だけをUIへ置かない。

