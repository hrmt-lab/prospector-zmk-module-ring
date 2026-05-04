# Claude Code 引き継ぎドキュメント — RING Dark Theme 実装

このドキュメントは、RINGレイアウトのダークテーマ（Midnight）をClaude Codeで実装するための引き継ぎ資料です。

---

## このタスクの背景

RINGレイアウト（ライトテーマ）の実装が完成し、次のステップとしてダークテーマを追加する。ダークテーマはライトテーマの「色だけを変えたもの」であり、**レイアウト・座標・フォント・アニメーション・ロジックは一切変更しない。**

---

## 重要な前提

- 既存のRING（ライト）レイアウトコードには**一切手を加えない**
- ダークテーマは**新しいKconfigエントリ**（例：`ZMK_DISPLAY_STATUS_SCREEN_LAYOUT_RING_DARK`）として追加
- カラートークンを `#define` で分岐させる方式を推奨（`ring_theme.h` 等）
- ライト/ダークでソースを最大限共有し、差分を最小化する

---

## 同梱ファイル

| ファイル | 内容 |
|---|---|
| `prospector-ring-dark-spec.md` | ダークテーマ仕様書（カラートークン・実装方針） |
| `mockup-dark.png` | ライトとダークの並列比較モックアップ |
| このファイル | 実装手順と依頼内容 |

**ライトテーマの仕様書（`prospector-battery-rings-spec.md`）も合わせて参照すること。** 座標・サイズ・フォント・アニメーションはすべてそちらに記載されている。

---

## 変更が必要なカラートークンのみ

ダークテーマで変わるのは以下5つのトークンのみ。

| トークン | Light | Dark |
|---|---|---|
| `bg.primary`（背景） | `#FAFAFA` | `#22282C` |
| `text.primary`（主要テキスト） | `#22282C` | `#E9E7E1` |
| `text.secondary`（ラベル類） | `#5F6A70` | `#929FA7` |
| `text.off`（MOD/STATE OFF文字） | `#D8DCDF` | `#3A4248` |
| `ring.track`（トラック・区切り線） | `#E2E5E8` | `#3A4248` |

以下は**変更しない**（Light/Dark 共通）：

| トークン | 値 |
|---|---|
| `accent` | `#E89B5C` |
| `ring.p1` | `#7A8B92` |
| `ring.p2` | `#A6B2B8` |
| `ring.p3` | `#C4CCD1` |

---

## 推奨実装方式

### 方式A: ヘッダファイルで分岐（推奨）

```c
// ring_theme.h
#pragma once
#include <lvgl.h>

#ifdef CONFIG_ZMK_DISPLAY_STATUS_SCREEN_LAYOUT_RING_DARK
  #define RING_BG           lv_color_hex(0x22282C)
  #define RING_TEXT_PRIMARY lv_color_hex(0xE9E7E1)
  #define RING_TEXT_SEC     lv_color_hex(0x929FA7)
  #define RING_TEXT_OFF     lv_color_hex(0x3A4248)
  #define RING_TRACK        lv_color_hex(0x3A4248)
#else
  // Light theme (default / RING)
  #define RING_BG           lv_color_hex(0xFAFAFA)
  #define RING_TEXT_PRIMARY lv_color_hex(0x22282C)
  #define RING_TEXT_SEC     lv_color_hex(0x5F6A70)
  #define RING_TEXT_OFF     lv_color_hex(0xD8DCDF)
  #define RING_TRACK        lv_color_hex(0xE2E5E8)
#endif

// 共通トークン
#define RING_ACCENT  lv_color_hex(0xE89B5C)
#define RING_RING_P1 lv_color_hex(0x7A8B92)
#define RING_RING_P2 lv_color_hex(0xA6B2B8)
#define RING_RING_P3 lv_color_hex(0xC4CCD1)
```

`ring_status_screen.c` 内のすべての色指定を `lv_color_hex(0x...)` の直書きから上記 `#define` に置き換える。ライト/ダーク両方で同じ `.c` ファイルを使い、Kconfigで分岐させる。

### 方式B: 別ファイルで独立実装

既存のライトテーマコードを `ring_light_status_screen.c` にリネームし、ダークテーマ用に `ring_dark_status_screen.c` をコピーして色定数だけ変える。シンプルだがコードの重複が多い。

**推奨は方式A。** 将来のレイアウト変更が1ファイルだけで済む。

---

## Kconfig への追加

```kconfig
choice ZMK_DISPLAY_STATUS_SCREEN_LAYOUT
    # 既存エントリはすべてそのまま
    ...

    # 新規追加のみ
    config ZMK_DISPLAY_STATUS_SCREEN_LAYOUT_RING_DARK
        bool "RING (Dark)"
        help
          RING layout with Midnight dark theme (WANA palette).
          Layout and logic are identical to RING (Light).
endchoice
```

CMakeLists.txt への追加：

```cmake
target_sources_ifdef(CONFIG_ZMK_DISPLAY_STATUS_SCREEN_LAYOUT_RING_DARK app PRIVATE
  ring_status_screen.c   # ライトと同じソースを使い回す（方式Aの場合）
)
```

---

## Claude Code への最初の依頼例

```
RINGレイアウト（ライトテーマ）のダークテーマ版を実装したい。

【前提】
- 既存のRING（ライト）コードには手を加えない
- 新しいKconfigエントリ RING_DARK として追加する
- カラートークンを ring_theme.h で #define 分岐させる方式を推奨

【変更するのはカラートークン5つだけ】
bg.primary:    #FAFAFA → #22282C
text.primary:  #22282C → #E9E7E1
text.secondary:#5F6A70 → #929FA7
text.off:      #D8DCDF → #3A4248
ring.track:    #E2E5E8 → #3A4248

accent / ring.p1 / ring.p2 / ring.p3 は共通（変更なし）

まず以下を読んでほしい：
- prospector-ring-dark-spec.md（ダークテーマ仕様書）
- mockup-dark.png（ライト/ダーク比較モックアップ）
- このドキュメント

その上で：
1. 現在のRINGライト実装コードを読んで、色の直書き箇所を洗い出してほしい
2. ring_theme.h を新規作成し、ライト/ダーク両方のトークンを定義してほしい
3. ring_status_screen.c の色直書きを ring_theme.h の #define に置き換えてほしい
4. Kconfig と CMakeLists.txt に RING_DARK エントリを追加してほしい
5. ビルドして動作確認してほしい
```

---

## 改訂履歴

| 日付 | 内容 |
|---|---|
| 2026-05-04 | 初版作成 |
