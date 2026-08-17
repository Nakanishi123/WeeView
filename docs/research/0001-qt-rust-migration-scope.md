---
vibedoc: 1
id: RES-0001
kind: research
tags:
  - migration
  - qt
  - product-spec
related:
  - DEC-0001
  - DEC-0002
  - DEC-0003
  - RES-0003
---

# Qt版からRust/GPUI版へ移植する対象

## 目的

Qt/C++版の実装構造ではなく、利用者から見える機能と挙動をRust/GPUI版へ移植するため、情報源と差分を整理する。

## 情報源の優先順位

仕様を判断するときは、次の分類を明示する。

| 分類 | 意味 |
|---|---|
| Existing | Qt版コードで動作を確認できる |
| Documented | `old/docs/` に記載されているが、コードと一致するとは限らない |
| Changed | Rust版で意図的に変更する |
| New | Rust版で追加する |
| Deferred | 製品仕様には含めるが、初期実装を延期できる |
| Non-goal | 実装しない、または高度化しない |

文書とQt版コードが衝突する場合は、コードを無条件に正解とはしない。意図的に議論した挙動は製品Decisionとして記録する。

## Existingとして確認できた主な挙動

- Single PageとSpreadがある。
- Right-to-Leftが既定である。
- Spreadは固定した奇数・偶数ペアではなく、移動方向を含む局所的なグループを作る。
- 横長ページはSpreadで単独表示する。
- `Home` は先頭ページからForward groupを作る。
- `End` は最終ページからForward groupを作るため、Spreadでも最終ページ単独になる。
- 通常の左クリックはMouse down時、通常の右クリックはMouse up時にページ移動する。
- 右ドラッグの形状が既知のCommandと完全一致した場合だけMouse gestureを実行する。
- Historyは表示グループの先頭Index、末尾Index、生成方向を保存する。
- Historyはページ名や書庫内Entry pathを保存していない。
- Natural sortは大文字小文字を区別せず、通常の数字と `〇`、`零`、`一`〜`九`、`十`、`百`、`千`、`万` を数値として扱う。
- Folder本の日時Sortでは、作成日時がなければMetadata change timeを使う。
- ZIP/CBZはlibzip、7z/CB7はlibarchiveで読み出している。

## Rust版で意図的に変更・追加するもの

- RustとGPUIを使う。
- Zoomは `Ctrl + Wheel` のPointer-centered zoomだけを提供する。
- Panは `Ctrl + 左ドラッグ` だけを提供する。
- Zoom/Panは現在の表示グループだけに属する一時状態とし、保存しない。
- Renameを追加する。ただし、明示的なRename以外では利用者のコンテンツを変更しない。
- HistoryにPage IDを追加し、ページ構成変更後も同じ画像を優先的に復元する。
- 設定・履歴・フォルダ別SortはSQLiteへ保存し、複数プロセス起動に対応する。
- 現在の本とSidebar folderのFile watcherを製品仕様に含める。
- Decoded image cacheの既定上限を512 MiB、Archive entry byte cacheを128 MiBにする。
- 壊れた個別画像をページ列から除外せず、エラーページとして残す。

## DeferredとNon-goal

- EXIF Orientationは対応方針だが、初期実装が複雑ならTaskとして延期できる。
- File watcherは製品仕様だが、初期実装を延期できる。
- 7z/CB7は対応するが、Solid archive専用最適化やDisk extraction cacheは実装しない。
- RAR/CBRは初期対象外とする。
- Fullscreen表示は実装しない。WindowのMaximizeは実装する。
- Delete、Move、書庫内部Rename、書庫編集、画像変換は実装しない。
- Qt版のクラス構成、QObject ownership、Widget階層は移植対象にしない。

## 関連文書

- [製品範囲と段階](../decisions/product/0001-product-scope-and-stages.md)
- [Viewerと見開き](../decisions/product/0002-viewer-and-spread-behavior.md)
- [入力・Zoom・Pan](../decisions/product/0003-input-zoom-and-pan.md)
