---
vibedoc: 1
id: TASK-0010
kind: task
status: todo
tags:
  - gpui
  - poc
  - viewer
  - performance
related:
  - RES-0002
  - DEC-0001
  - DEC-0003
  - ARCH-0003
depends_on: []
---

# GPUI Viewer PoCを実施する

## 目的

WeeView全体を作る前に、巨大画像のBackground decode、GPU描画、Zoom/Pan、Page switch、CacheがGPUI 0.2.2で実用になることを確認する。

## Scope

- Local folderからJPEG、PNG、WebP、AVIFを数枚読み込む。
- 3000×5000画像を含める。
- Background decodeと最大2並列制御を作る。
- Decode結果をGPUI `RenderImage` として描画する。
- Checkerboard、Fit Window、`Ctrl + Wheel` Zoom、`Ctrl + 左ドラッグ` Panを作る。
- Page switch、120 ms debounce、Generation、512 MiB decoded cacheを作る。
- Debug instrumentationでDecode、RenderImage生成、Cache Hit/Miss/Evictionを確認する。
- Client decorationのmove、edge resize、maximize/restoreをWindows、X11、Waylandで確認できる小さい検証を作る。

## Scope外

- Archive、Sidebar、SQLite、History、Rename、Watcher、Thumbnail、完成UI。
- 7z performance。
- Shadowや角丸のPlatform間一致。

## Acceptance

- JPEG、PNG、WebP、AVIFを表示できる。
- 3000×5000画像をAspect ratio維持でFit Window表示できる。
- Decode中もWindow操作と入力がBlockしない。
- Zoom/Panごとに再DecodeまたはCPU resize画像を生成しない。
- Pointer-centered zoomがClamp可能な範囲で注視点を維持する。
- PanがViewport境界へClampされる。
- Page変更とViewport resizeでFit Windowへ戻る。
- Cache済みPageを新規Decodeせず再表示できる。
- Current groupをEvictしない。
- 高速移動の古いResultで表示が巻き戻らない。
- Cache MissだけをDebounceし、Cache Hitは即時表示できる。
- Decode失敗をPanicにせずError stateとして返せる。
- Release buildでZoom/Panに目立つ停止がない。
- Windows、X11、WaylandでWindow move、resize、maximize/restoreの可否を記録する。

## 完了時

- PoC結果をResearch文書へ追記または新規作成する。
- `RenderImage` と元Pixel bufferの二重保持が必要かを記録する。
- GPUIで成立しないAcceptanceがあれば、本実装前にArchitecture Decisionを更新する。

