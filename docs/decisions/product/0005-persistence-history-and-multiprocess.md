---
vibedoc: 1
id: DEC-0005
kind: decision
status: accepted
tags:
  - sqlite
  - history
  - settings
  - multiprocess
related:
  - DEC-0002
  - DEC-0004
  - ARCH-0003
---

# Persistence・History・複数プロセス

## SQLite

Settings、Folder sort、Historyを1つのSQLite databaseへ保存する。JSON全体を複数プロセスが上書きする方式は採用しない。

主なTableは `meta`、`settings`、`folder_sorts`、`history` とする。画像、GPU texture、Archive byte cache、Thumbnail本体、一時UI状態はDBへ保存しない。

- WAL modeを使う。
- Busy timeoutは既定5秒程度とする。
- SQLite処理をUI threadで行わない。
- 1プロセスにつきPersistence commandの書き込み経路を1つにする。
- 複数Row更新はTransactionにする。
- DBを開けない場合は勝手に削除せず、Persistenceなしで起動可能にしてエラーを表示する。

## 複数プロセス

- 複数のWeeView processを同時起動できる。
- 既存ProcessへのOpen path転送は行わない。
- 各Processは独立したWindowと現在の本を持つ。
- 異なる本はRow単位のUPSERTにより互いの進捗を消さない。
- 同じ本を複数Processで更新した場合は最終更新が勝つ。
- Settingsは変更したKeyだけを更新する。
- 他Processの変更を起動中のUIへRealtime pushしない。History/Settings View表示時またはReload時に再取得する。
- Window状態は最後に正常保存したWindowの値を次回起動Defaultにする。

## History

最大200冊のUnique book pathを保持し、`last_opened_at` が新しい順とする。

各Entryは少なくとも次を持つ。

- Book path、Book type、Display name。
- Display groupの先頭・末尾Page index。
- Display groupの先頭・末尾Page ID。
- Page count、View mode、Reading direction、Group direction。
- UTCの最終閲覧日時。

Folder本のPage IDはFile名、Archive本はフルEntry pathを使う。復元時はPage IDを優先し、存在しなければ保存Indexを現在Page countへClampする。先頭・末尾IDの両方が一致しなければ、復元した先頭から通常のDirectional local groupingを再計算する。

0ページの本はHistoryへ追加しない。Missing pathのHistoryは自動削除しない。

## Qt JSON import

新DBが空でLegacy import未実行なら、Qt版の `settings.json` と `history.json` を読み込む。

- ImportはTransactionで行う。
- 元JSONを変更・削除しない。
- Qt版の先頭・末尾IndexとGroup directionを引き継ぐ。
- Page IDはNoneでImportし、その本を正常に開いた時点で補完する。
- 旧Cache設定が明示されていれば尊重し、未設定時だけ新Defaultを使う。
- 複数Processの同時初回起動はMeta flagとUnique制約で二重Importを防ぐ。

## Defaultsと不正値

不正値を範囲へClampせず、そのFieldだけDefaultへ戻す。正常Fieldは維持し、未知Fieldは無視する。

主なDefaultは次のとおり。

| 項目 | Default |
|---|---:|
| Reading direction | Right-to-Left |
| View mode | Single Page |
| Overlay trigger | 24 px |
| Overlay hide delay | 800 ms |
| Page load debounce | 120 ms |
| Decoded image cache | 512 MiB |
| Archive byte cache | 128 MiB |
| Sidebar width | 320 px |
| Window size | 960×720 |
| File watcher | enabled |

本にHistoryがあればView modeとReading directionをHistoryから復元し、なければSettingsのDefaultを使う。Zoom/Panは常にFit Windowから始める。

## 保存タイミング

- メモリ上のViewer/Settings状態は即時更新する。
- 通常のDB保存は約500 ms Debounceする。
- 本の切替、Rename成功、History削除、終了時は即時Flushを試みる。
- Busyの場合は未保存状態を保持し、短いBackoffで再試行する。
- 保存できたと偽らず、最終的な失敗を通知・Logする。

