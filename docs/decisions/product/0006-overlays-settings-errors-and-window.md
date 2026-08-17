---
vibedoc: 1
id: DEC-0006
kind: decision
status: accepted
tags:
  - overlay
  - settings-ui
  - errors
  - window
related:
  - DEC-0004
  - DEC-0005
  - TASK-0010
---

# Overlay・Settings・Error・Window

## Overlay

Header、Footer、Sidebarは画像領域を押し縮めず、その上へ重なる。既定のEdge triggerは24 px、Hide delayは800 msとする。

- 対応する端へPointerが入ると表示する。
- Overlay本体へPointerがある間は維持する。
- 離れたら独立したHide timerを開始し、戻ればCancelする。
- 複数Overlayを同時表示できる。
- SidebarをHeader/Footerより前面へ置く。
- Windowが非Activeならすべて隠す。
- Context menuやDialog中は関連Overlayを維持する。
- Mouse button押下、Pan、Gesture、Slider操作中は新しいEdge triggerを発火せず、勝手にHideしない。

## Header

- Book path、Single/Spread、RTL/LTR、Minimize、Maximize/Restore、Closeを表示する。
- Book未選択時はアプリ名を表示する。
- 長いPathは中央省略し、Tooltipで全文を示す。
- 空白部分のDragでWindowを移動する。
- 空白部分のDouble clickでMaximize/Restoreする。
- Button上のDragはWindow移動に使わない。
- Close hit targetは右上隅まで届く。

## Window

- WindowsとLinuxでFrameless Windowを使う。
- Window edge resize、Minimize、Maximize、Restore、Closeを実装する。
- Fullscreenは実装しない。
- 通常BoundsとMaximized状態を保存する。
- 復元時は接続中Monitorの表示領域へClampする。
- Shadowと角丸のPlatform間完全一致は要求しない。

## Settings View

General、Viewer、Overlay、Sidebar、Windowへ分ける。

- Home folder、Default RTL/LTR、Default Single/Spread。
- 実装後のFile watcher toggle。未実装中は動かないToggleを表示しない。
- Page load debounce、Decoded image cache、Archive byte cache。
- Cache clear。現在表示Groupは残す。
- Overlay triggerとHide delay。
- Sidebar幅を320 pxへ戻す。
- Folder sortをすべて削除する。
- Window size/positionを次回起動時にDefaultへ戻す。
- Section resetと全Settings reset。

全Settings resetでもHistoryを削除しない。Folder sort resetと全Settings resetは確認する。Toggleや選択は即時反映し、数値はEnterまたはFocus移動で確定する。不正値はその項目のDefaultへ戻す。

## Loading

- 新しい本の索引作成中は現在の本を維持し、本名とLoading indicatorを表示する。
- 索引と初期Groupが決まった時点で本を切り替え、最初の画像Decode完了は待たない。
- 高速ページ移動中は最後の正常画像を維持し、予定Pageまたは範囲を中央表示する。
- 表示可能画像がなければCheckerboard上へLoadingを表示する。
- Metadata待ちとDecode待ちは内部で分けるが、UI文言は共通でよい。

## Error

### Page error

個別画像のDecodeまたはArchive Entry読み出し失敗はViewer内のエラーページにする。Page名、短い理由、再試行を表示し、前後移動は可能にする。

### Toast

本を開けない、Rename失敗、保存失敗、Watcher reload失敗など、作業を継続できるErrorはToastで示す。同種の連続Errorはまとめ、保存失敗は通常より長く表示する。

### Dialog

上書き衝突、非対応拡張子へのRename、Settings resetなど、利用者の選択が必要な場合だけDialogを使う。

## Log

Book open、Archive/Decode error、古いRequest結果の破棄、Cache eviction、Watcher event、SQLite migration/save、Renameを構造化Logへ記録する。通常のPage移動やCache Hit/MissはDebug levelとし、Info logを大量に出さない。

