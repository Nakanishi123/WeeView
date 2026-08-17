---
vibedoc: 1
id: ARCH-0003
kind: architecture
status: accepted
tags:
  - async
  - image
  - cache
  - performance
related:
  - RES-0002
  - DEC-0002
  - DEC-0003
  - TASK-0010
---

# 非同期画像PipelineとCache

## Pipeline

```text
UI Action
  ↓
Application load coordinator
  ├─ Book index task
  ├─ Metadata task
  ├─ Decode task
  └─ Preload task
       ↓
Typed result + Book ID + Generation
       ↓
Current requestならUIへ反映
```

汎用Actor frameworkや全Application共通のEvent busは導入しない。用途ごとの小さいCommand/Result enumを使う。

## Generation

Book openとPage loadには、少なくともBook IDと単調増加するGenerationを付ける。

- 現在Generationと一致するResultだけを現在表示へ反映する。
- 同じ本の古いDecode結果は、まだ有用ならCacheへ格納できる。
- 別の本のResultは破棄する。
- Task cancelが可能なら行うが、Cancel不能でも古いResultが表示を壊さないことを必須とする。
- Result破棄理由をDebug logへ残せる。

## Priorityとdebounce

優先順位は次のとおり。

1. 現在Groupを決定するMetadata。
2. 現在Groupの画像。
3. 次のGroup。
4. その次のGroup。
5. 前のGroup。
6. その他のPreload。

Page stateは入力時に即時更新する。Cache Hitは即表示し、Cache Missの新規Decodeだけを既定120 ms Debounceする。Decode同時実行数は2とし、未開始のPreloadより現在要求を優先する。

## Decoded image

Decode結果はGPUIが描画できるBGRA8等の一貫した形式へ変換する。Cache byte sizeは実際の展開済みBufferから計算する。

GPUI `RenderImage` がCPU Frame dataも保持するため、元RGBA/BGRA bufferと `RenderImage` を長期的に二重保持しない。PoCで必要性を確認し、Cache entryが最小限の描画資源を所有する。

## Decoded image cache

- Default上限は512 MiB。
- 現在表示GroupはEvictionしない。
- 現在Groupだけで上限を超えても表示を維持する。
- 残予算の約2/3を論理的な次方向、約1/3を前方向へ使う。
- 配分はPage数ではなくDecoded byte sizeを基準にする。
- 一方が予算を使わなければ他方へ回せる。
- Eviction時はCPU dataと対応GPUI imageの参照を解放する。
- GPU driver内部の遅延解放は上限保証外とする。

Cache clearでは現在Groupを残す。上限縮小時も同じ保護規則で直ちにEvictionする。

## Archive byte cache

- Default上限は128 MiB。
- Decoded image cacheとは別予算にする。
- ZIPと7zで共通の単純なCache policyを使う。
- 7z専用Disk cacheやSolid archive最適化を実装しない。
- Cache Miss時に7zを先頭から走査する遅さは既知の制約とする。

## 作業中メモリ

512 MiBと128 MiBはProcess RSS全体の上限ではない。Decode作業Buffer、GPU texture、Library内部Buffer、UIなどは別に存在する。Decode同時実行数2は作業中メモリの急増も抑える。

## Error

- 対応拡張子のDecode失敗はPageを削除せずError stateへする。
- Error Pageは再試行できる。
- 失敗結果を永久Cacheしない。
- Archive全体を開けないErrorと個別Entry errorを区別する。
- Panicや空画像による暗黙の成功へ変換しない。
