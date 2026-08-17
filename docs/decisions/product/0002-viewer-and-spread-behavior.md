---
vibedoc: 1
id: DEC-0002
kind: decision
status: accepted
tags:
  - viewer
  - spread
  - history
related:
  - DEC-0003
  - DEC-0005
  - ARCH-0002
---

# Viewerと見開きの挙動

## 基本概念

- 1画像を1ページとする。
- Page indexは内部で0-basedとする。
- Single Pageを既定とする。
- Right-to-Leftを既定とする。
- 画像はAspect ratioを維持し、Fit Windowで中央表示する。
- 透明部分を確認できる黒とDark grayのCheckerboardを背景に使う。
- 横長判定は `width > height` とする。
- EXIF Orientationへ対応した場合は、Orientation適用後の寸法で横長判定する。

## Display group

現在位置は単なるPage indexではなく、次を持つDisplay groupとして扱う。

- 先頭Page index。
- 末尾Page index。
- ForwardまたはBackwardの生成方向。

Single PageのDisplay groupは常に1ページである。Spreadは1ページまたは連続する2ページである。

## Directional local grouping

Spreadは固定した奇数・偶数ペアやSpread anchorを使わない。

### Forward group

Page `N` から開始する。

- `N` が横長なら `[N]`。
- `N + 1` が存在しなければ `[N]`。
- `N + 1` が横長なら `[N]`。
- それ以外は `[N, N + 1]`。

### Backward group

Page `N` で終了する。

- `N` が横長なら `[N]`。
- `N - 1` が存在しなければ `[N]`。
- `N - 1` が横長なら `[N]`。
- それ以外は `[N - 1, N]`。

### Navigation

- Nextは現在Groupの末尾の次からForward groupを作る。
- Previousは現在Groupの先頭の前で終わるBackward groupを作る。
- 過去に通ったGroupをStackへ保存しない。
- 横長ページ周辺では前進と後退が非対称になり得る。これは仕様である。

## Mode切替

- SingleからSpreadへ切り替えると、現在Page indexを先頭とするForward groupを作る。
- SpreadからSingleへ切り替えると、現在Groupの先頭ページを表示する。
- Reading direction変更は論理Groupを変えず、視覚的な左右だけを変える。

## 視覚順

- RTLでは高いPage indexを左、低いPage indexを右へ置く。
- LTRでは低いPage indexを左、高いPage indexを右へ置く。
- 2ページは中央Seamへ寄せ、上下は中央揃えにする。
- 見開き全体を1つの表示CanvasとしてZoom/Panする。

## HomeとEnd

Qt版の挙動を維持する。

- `Home` は先頭ページからForward groupを作る。
- `End` は最終ページからForward groupを作る。
- したがってSpreadでも `End` は最終ページ単独になる。

## Metadata待ち

- 本を開く際に全ページを同期Decodeしない。
- 現在Groupの判定に必要なPage metadataを最優先で取得する。
- Metadata不明ページを仮に縦長扱いしたSpreadを表示しない。
- 必要なMetadataが揃うまでは直前の正常画像またはLoading表示を維持する。

## 読書状態

- Historyがなければ未読。
- Historyがあり、現在Groupが最終ページを含まなければ読書中。
- 現在Groupの末尾が本の最終ページなら読了。
- 読了後に前へ戻れば読書中へ戻す。
- 永続的な「一度読了した」Flagは持たない。

## Footer表記

- Singleは `10 / 100`。
- Spreadは `10–11 / 100`。
- SliderはPage indexを値とし、指定IndexからForward groupを作る。
- RTLでは右端が先頭、LTRでは左端が先頭になる。

