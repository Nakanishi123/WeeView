---
vibedoc: 1
id: RES-0003
kind: research
tags:
  - product-spec
  - open-questions
related:
  - RES-0001
  - DEC-0001
  - ARCH-0003
---

# 未決の製品仕様

## 目的

現時点で壁打ちしていない事項を、確定仕様へ暗黙に混ぜないために記録する。実装前に影響範囲に応じてProduct DecisionまたはTaskへ移す。

## 画像

### Animated WebP

WebP対応は確定しているが、Animated WebPを再生するか、先頭Frameだけを静止画表示するかは未決である。Qt版の実挙動と、漫画Viewerとしての期待を確認する。

### Color profile

ICC profile、sRGB変換、HDR画像の扱いは未決である。初期版でOS/GPUと `image` crateの既定へ任せる場合も、既知の制約を記録する。

### 安全上限

極端な寸法、展開後容量、Archive entry size、Decode bombに対する拒否上限は未決である。Cache上限だけではDecode作業中メモリを制限できない。

## File列挙

### Hidden file

DotfileやOSのHidden属性をFiles ViewとFolder本へ含めるかは未決である。`.DS_Store` と `__MACOSX/` の除外だけは確定している。

### Unicode正規化

Unicode正規化形式が異なるFile名のSort、Page ID比較、Rename衝突判定は未決である。File system操作ではOSが返すPathを正とする必要がある。

## Archive

### Password・暗号化

暗号化ZIP/7zのPassword入力UIを提供するか、対応外Errorにするかは未決である。

### File名Encoding

UTF-8でないZIP entry nameのFallback範囲は未決である。Qt版はUTF-8を優先し、取得できない場合にlibzipの既定取得を試すが、Rust版の対応をCharacterizationする必要がある。

### 重複Entry名

同じフルEntry pathを複数持つArchiveでPage IDをどう一意化するかは未決である。Entry indexをPage IDへ含める案がある。

## 起動とPlatform統合

### Command line

複数Pathを引数で渡した場合に最初の対応Pathだけを開くQt版仕様を維持するかは未決である。複数Process対応と組み合わせ、PathごとにWindowを開く案もある。

### UI言語

Qt版は日本語Labelを使う。Rust版を日本語固定にするか、最初から翻訳可能な文字列管理だけ用意するかは未決である。

### Packaging

Windows installer、File association、Linux desktop entry、AppImage等の配布形式は未決である。

