---
vibedoc: 1
id: ARCH-0002
kind: architecture
status: accepted
tags:
  - domain
  - viewer
  - state-machine
related:
  - DEC-0002
  - DEC-0003
  - ARCH-0001
---

# DomainとViewer state

## 型

生のIntegerやStringへDomain上の意味を背負わせず、次のような型を使う。

```text
PageIndex
PageId
BookId
BookPath
BookType
ViewMode
ReadingDirection
GroupDirection
DisplayGroup
ReadingState
SidebarSortKey
SidebarSortOrder
```

`PageIndex` は0-basedであり、本のPage countに対する検証を明示的に行う。UIへ表示するときだけ1-basedへ変換する。

## Page metadata

Pageは少なくとも次を持つ。

```text
id
display_name
source_location
metadata_state
```

Metadata stateは未取得、取得中、利用可能、失敗を区別する。利用可能なMetadataはOrientation適用後の幅、高さ、横長判定を持つ。

## Viewer position

永続化可能な位置と一時Transformを分ける。

```text
ViewerPosition
├── display_group
├── view_mode
└── reading_direction

ViewportTransform
├── fit_scale
├── zoom_scale
└── pan_offset
```

`ViewerPosition` はHistoryへ保存できる。`ViewportTransform` は表示Groupが変わると破棄し、保存しない。

## Display groupの不変条件

- Page countが0ならGroupは存在しない。
- Single Pageの先頭と末尾は等しい。
- Spreadの末尾は先頭または先頭+1である。
- Group内のIndexはBook範囲内である。
- `GroupDirection` はGroupを復元・再計算するために保持する。
- Reading directionはGroupのIndex順を変更しない。

Forward/Backward group計算、Next、Previous、Home、End、1 logical page stepは純粋関数として実装し、横長ページ周辺の非対称例をTable-driven testにする。

## Loading state

論理位置と描画可能画像を分ける。

```text
target_position
last_painted_group
load_status
```

高速移動では `target_position` を即時更新する。新Groupを描画できるまで `last_painted_group` を維持して予定Page overlayを表示する。古い画像を表示している間もHistoryへ保存する位置は論理的なTargetとする。

## History restore

復元は次の順とする。

1. 保存された先頭Page IDを現在のPage列から探す。
2. 見つからなければ保存IndexをClampする。
3. 先頭・末尾Page IDの両方とGroupが現在も整合すれば、そのGroupを復元する。
4. 整合しなければ先頭Pageから保存Directionに応じて再計算する。
5. 0ページなら位置なしとする。

旧HistoryでPage IDがない場合も同じIndex fallbackを使い、正常Open後にPage IDを補完する。

## Reading state

```text
履歴なし                         -> Unread
履歴あり、Group末尾 < 最終Index  -> Reading
Group末尾 == 最終Index           -> Completed
```

Completedは現在位置から導出し、独立した永続Flagにしない。
