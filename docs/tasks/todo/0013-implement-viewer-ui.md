---
vibedoc: 1
id: TASK-0013
kind: task
status: todo
tags:
  - gpui
  - viewer
  - input
  - window
related:
  - DEC-0002
  - DEC-0003
  - DEC-0006
depends_on:
  - TASK-0011
  - TASK-0012
---

# Viewer・入力・Overlay UIを実装する

## Scope

- Single/Spread描画とRTL/LTR。
- Keyboard、Wheel、Click、Zoom、Pan、Mouse gesture。
- Header、Footer、Loading、Page error、Toast、Dialog。
- Frameless move、edge resize、maximize/restore。
- Window bounds保存用Event生成。

## Acceptance

- Product Decisionどおりの入力確定タイミングになる。
- OverlayがPan/Gesture/Slider操作を妨げない。
- Fullscreenを追加しない。
- Render中に同期I/Oを行わない。
- Windows、X11、Waylandで必須Window操作を確認する。

