---
vibedoc: 1
id: DEC-0004
kind: decision
status: accepted
tags:
  - sidebar
  - files
  - rename
  - sorting
related:
  - DEC-0005
  - ARCH-0003
---

# Sidebar・File操作・Rename

## Sidebar View

Sidebarは左端のOverlayとし、Files、History、Settingsの3 Viewを持つ。起動時はFilesとし、現在Viewは永続化しない。

Sidebarの現在Directoryと現在開いている本は別状態である。Sidebarだけを移動してもViewerの本を変更しない。

幅は既定320 px、最小160 px、最大Window幅の80%とする。希望幅を保存し、Windowが狭い間だけ実幅をClampする。

## Files View

表示対象はDirectory、対応画像、対応Archiveだけとする。

- Directoryは昇順・降順にかかわらず常にFileより先へ置く。
- Directory群とFile群の内部に同じSort key/orderを適用する。
- 同値ならNatural filename、さらに同値ならPathで決定的に並べる。
- Sort keyはFilename、Created time、Modified time。
- Created timeがなければMetadata change time、さらに取得不能ならNatural filenameへFallbackする。
- Sort設定はDirectoryごとにSQLiteへ保存する。

Natural sortはQt版の観測可能な並びを踏襲する。大文字小文字を区別せず、通常の数字に加えて `〇`、`零`、`一`〜`九`、`十`、`百`、`千`、`万` からなる明確な漢数字列を数値Tokenとして扱う。同じ数値へ正規化された場合は元文字列でTie breakする。Rust版ではQtの `QCollator` 実装自体ではなく、代表的なFile名のCharacterization testを互換対象にする。

Folder本のページ順はそのDirectoryのSidebar sortへ従う。Sort変更後はPage IDで現在画像を維持し、Display groupを再計算する。Archive内部は常にフルEntry pathのNatural sortとする。

## Directory click

- Single clickはOSのDouble click判定時間が経過した後、そのDirectoryをFolder本として開く。
- Double clickは保留中のSingle clickをCancelし、Sidebarの現在Directoryを移動する。
- Image clickは親DirectoryをFolder本として開き、その画像へ移動する。
- Archive clickはArchive本を開く。

## Navigation

- Home、Back、Forward、Up、Reloadを提供する。
- Back/Forward historyはWindow内だけに保持し、永続化しない。
- Back後に新しいDirectoryへ移動した場合はForward historyを破棄する。
- RenameされたPathはBack/Forward history内でも更新する。
- 削除済みPathはNavigation時に飛ばす。
- Up後は移動元Directoryを選択し、表示範囲へScrollする。

ReloadはSidebar folderを再列挙する。同じFolder本が開かれていればPage IDを優先して再構築し、Archive変更も再Openする。Zoom/PanはFit Windowへ戻す。

## 前の本／次の本

現在の本の親Directoryから候補を作る。

- 直下に対応画像が1枚以上あるDirectory。
- ZIP、CBZ、7z、CB7。
- 画像File単体、空Directory、対応画像が孫DirectoryにしかないDirectoryは候補外。
- 順序はSidebar sortと同じで、Directoryを常に先へ置く。
- 端ではWrapしない。
- 候補を開けなくても次候補へ自動Skipせず、現在の本を維持してエラーを表示する。
- Directoryの候補判定はBackgroundで行う。

## Rename

対象はDirectory、対応画像、対応Archiveとする。SymlinkとArchive内部EntryはRenameできない。

- F2またはContext menuからInline Renameを開始する。
- Fileは開始時に拡張子を除くBase nameを選択するが、拡張子を含む全体を編集できる。
- Enterで確定、EscapeでCancel、Focus喪失でCancelする。
- 空文字、`.`、`..`、Path区切り文字を拒否する。
- 同名Entryを上書きしない。
- 非対応拡張子への変更は警告後に許可する。
- 成功後に再Sortし、Page IDで現在画像を追跡する。
- 失敗時は元名を維持し、理由を表示する。

明示的なRename以外で利用者コンテンツを変更しない。Rename成功後にDB更新へ失敗してもFile名を元へ戻さない。

Directory Renameでは、History、Folder sort、Home folder、Sidebar path、Back/Forward historyなどの配下PathをPath component単位で更新する。文字列の単純な前方一致は使わない。

## File watcher

製品仕様として次を監視する。

- 現在開いているFolder本。
- 現在開いているArchive file。
- Sidebarの現在Directory。
- Home folderそのものの移動・削除。

全History pathは監視しない。変更EventをDebounceして再列挙し、Page IDで現在画像を維持する。ArchiveはSizeと更新日時が安定してから再Openする。初期実装が複雑ならTaskとして延期できる。
