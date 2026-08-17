---
vibedoc: 1
id: ARCH-0004
kind: architecture
status: accepted
tags:
  - persistence
  - filesystem
  - ui
related:
  - DEC-0004
  - DEC-0005
  - DEC-0006
  - ARCH-0001
---

# Persistence・File system・UI境界

## Persistence worker

各ProcessはSQLiteへの書き込みを1つのPersistence command経路へ送る。

```text
PersistenceCommand
├── UpdateHistory
├── DeleteHistory
├── UpdateSetting
├── UpdateFolderSort
├── ResetSettings
└── RewritePathsAfterRename
```

読み出しもUI threadをBlockしない。通常更新は約500 ms Debounceし、同じHistory/Setting keyへの未送信Commandをまとめられる。Rename、削除、本切替、終了時はFlushを試みる。

DB SchemaはVersionを持ち、未知の新Versionを古いApplicationが上書きしない。MigrationとLegacy JSON importはTransactionで行う。

## Path identity

- File操作用Pathは `PathBuf` で保持し、早期に `String` へ変換しない。
- 正式サポートは有効なUnicodeへ変換できるPathとする。
- Symlinkを開いたHistoryはCanonical targetではなく、利用者が開いたLink側Pathを識別子にする。
- Symlink自体はRenameできない。
- Directory配下判定はPath componentで行い、文字列前方一致を使わない。
- OS固有の大小文字、予約名、末尾文字制約に従う。

## Rename workflow

```text
入力検証
  ↓
Symlink・衝突・OS制約確認
  ↓
File system rename
  ↓ 成功
Memory上のPathを新値へ更新
  ↓
SQLite Transactionで関連Pathを更新
  ↓
Sidebar/Page列を再構築
```

File systemとSQLiteを1つのAtomic transactionにはできない。DB更新失敗時もFile名を戻さず、Memory上の新Pathを正として再試行する。

## Watcher

Watcher eventは直接UI stateを書き換えず、Applicationへ通知する。

- Folder eventをDebounceして再列挙する。
- ArchiveはSizeと更新日時が一定時間安定してから再Openする。
- Page IDを使って現在Pageを維持する。
- Current request generationを更新し、旧Decode結果を無効化する。
- 再Open失敗時は最後の正常表示を維持して通知する。

## GPUI state

UIはDomain/Application stateの投影とAction dispatchを担う。

- Render中に同期I/Oを行わない。
- GPUI EntityへSQLite connectionやArchive handleを直接埋め込まない。
- Overlay visibility、Focus、Inline editor、ToastなどUI固有状態はUI Moduleへ置く。
- Viewer positionやReading stateの規則はDomainへ置く。
- Loading coordinatorとGeneration判定はApplicationへ置く。

## Windowと複数Process

1 Processにつき独立したWindowを持つ。別ProcessのHistory/Settings変更をRealtime同期しない。Viewを開いたときまたはReload時にSQLiteから読み直す。

Window boundsは最後に保存したProcessが勝つ。復元時に現在DisplayへClampする。Fullscreen stateは持たず、Windowed/MaximizedとRestore boundsだけを扱う。
