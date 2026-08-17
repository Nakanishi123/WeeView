---
vibedoc: 1
id: DEC-0003
kind: decision
status: accepted
tags:
  - input
  - zoom
  - pan
related:
  - DEC-0002
  - TASK-0010
---

# 入力・Zoom・Pan

## Keyboard navigation

| 入力 | 操作 |
|---|---|
| `Space` / `PageDown` | 次のDisplay group |
| `Backspace` / `PageUp` | 前のDisplay group |
| `Home` | 先頭からForward group |
| `End` | 最終ページからForward group |
| `←` / `→` | Reading direction依存 |

RTLでは左矢印が次、右矢印が前である。LTRでは逆になる。Key repeatはOSへ従い、論理状態を即時更新する。

Inline Rename、Settings入力、Context menu、Dialogの操作中は背後のViewer shortcutを実行しない。Sidebar listにFocusがある場合、矢印キーはRow選択へ使う。

## Mouse navigation

- 通常Wheel downは次、upは前。Reading directionには依存しない。
- Trackpadの微小Deltaは蓄積し、閾値を超えた場合だけページ移動する。
- RTLでは左クリックが次、右クリックが前。
- LTRでは左クリックが前、右クリックが次。
- Qt版どおり、通常左クリックはMouse down時、通常右クリックはMouse up時に確定する。
- Overlay、Window resize border、Dialog上の入力をViewerへ伝播しない。

## Zoom

- Zoom操作は `Ctrl + Wheel` だけとする。
- Pointer-centered zoomとし、Pointer下の画像座標を可能な範囲で同じ画面位置へ保つ。
- `+`、`-`、100%、Fit Width、Fit Heightは提供しない。
- Fit Windowを最小倍率とし、最大1000%とする。
- Zoomごとに再DecodeまたはCPU resize画像生成を行わない。

Zoomは現在のDisplay groupだけに属する一時状態である。次の場合に破棄してFit Windowへ戻す。

- PageまたはDisplay group変更。
- 本の変更。
- Single/Spread変更。
- RTL/LTR変更。
- Viewport resize。
- ReloadまたはSortによるページ列再構築。

Zoom倍率とPan位置はSettingsやHistoryへ保存しない。以前Zoomしたページへ戻っても復元しない。

## Pan

- `Ctrl + 左ドラッグ` だけを使う。
- `Ctrl + 左Mouse down` ではページ移動しない。
- Drag閾値を超えたらPanを開始する。
- `Ctrl + 左クリック` は何もしない。
- Pan開始後にCtrlを離しても、そのDragが終わるまでPanを継続する。
- Viewportを超えた軸だけ移動できる。
- 画像Canvasを完全にViewport外へ移動できないようClampする。
- Viewportより小さい軸は中央固定する。

## 右ボタンGesture

| 形状 | 操作 |
|---|---|
| `→ ←` | 論理Pageを1ページ進む |
| `← →` | 論理Pageを1ページ戻る |
| `↑ →` | 最初のページ |
| `↑ ←` | 最後のページ |
| `↓ →` | 前の本 |
| `↓ ←` | 次の本 |

- Movement thresholdは40 logical px、Dominance ratioは1.25を既定とする。
- 同じ方向の連続Segmentは1つにまとめる。
- 入力中は矢印列を中央表示し、完全一致時だけCommand名も表示する。
- Mouse up時の完全一致だけを実行する。
- 余分なSegmentまたは曖昧な移動があれば実行しない。
- Dragが発生した場合、不一致でも通常右クリックを実行しない。
- Gestureの形状と意味はReading directionに依存しない。

「論理Pageを1ページ進む」は次のDisplay groupへ移動する操作とは異なる。Group先頭を1だけ進め、そこからForward groupを作る。

