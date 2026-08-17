---
vibedoc: 1
id: RES-0002
kind: research
tags:
  - gpui
  - poc
  - performance
related:
  - ARCH-0001
  - ARCH-0002
  - TASK-0010
---

# GPUI 0.2.2の移植前調査

## 調査対象

`Cargo.toml` で固定されている `gpui 0.2.2` のローカルCrate sourceとExampleを確認した。

## 確認できた機能

- `RenderImage` はBGRA形式の `image::Frame` を保持する。
- `RenderImage` には画像ごとの `ImageId` があり、同じ画像を描画時に識別できる。
- `paint_image` で指定Boundsへ画像を描画できる。
- 標準 `img` elementはAVIFを対応拡張子として列挙する。
- GPUIが使う `image` crateの依存にはAVIF decoderが含まれる。
- Background executorがある。
- X11とWayland featureが既定で有効である。
- Client-side decoration、Window move、Window edge resize、Maximized boundsを扱うAPIとExampleがある。

## WeeViewが標準 `img(path)` へ任せないもの

WeeViewはDecodeとCacheを自分で管理し、Decode済み結果から `RenderImage` を作る。

理由は次のとおり。

- Decode同時実行数を2へ制限する。
- 現在ページを優先し、プリロードを低優先度にする。
- Decoded image cacheを512 MiBで管理する。
- 古いRequest結果を現在表示へ反映しない。
- 書庫内のBytesから同じ経路でDecodeする。
- Cache Hit、Miss、Evictionを計測する。
- 画像エラーをWeeViewのDomain errorへ統一する。

## PoCで未確認の事項

- 3000×5000画像をZoom/Panしたときの実際のFrame pacing。
- 同じ `RenderImage` のZoom/PanでGPU Uploadが不要に繰り返されないこと。
- `RenderImage` 以外にRGBA bufferを長期保持する必要があるか。
- AVIFのDecode時間と作業中メモリ。
- GPUI Background executorでCPU-bound decodeを実行する構成の妥当性。
- Windows、X11、WaylandでのFrameless move、resize、maximize、restore。
- High-DPI環境でのPointer-centered zoom座標変換。
- TrackpadのWheel deltaがPlatformごとにどう通知されるか。

これらは [GPUI Viewer PoC](../tasks/todo/0010-gpui-viewer-poc.md) の合格条件として実証する。

