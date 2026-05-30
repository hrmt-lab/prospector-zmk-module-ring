# RING — AI Usage 画面 UI 仕様書

ZMK ドングル向け RING レイアウトの AI 使用率モニター画面の UI 仕様書。

RING メイン画面と LVGLオブジェクトを共有し、AI Usage 固有の要素のみを
表示時に生成・離脱時に破棄する軽量設計を採用する。

---

## 1. 概要

### 1.1 目的

以下の情報を1画面に表示する。

- 現在のアクティブレイヤー名
- 修飾キーの押下状態（CTRL / SHIFT / ALT / GUI）
- IME 状態（EN/JP）
- ディスプレイ輝度
- claude_code / codex の使用率（5時間枠・7日枠）

### 1.2 対象ディスプレイ

| 項目 | 値 |
|---|---|
| 解像度 | 280 × 240 px |
| 画面形状 | 横長 |
| カラー | フルカラー |

### 1.3 設計方針

- RING メイン画面と同じカラートークンを使用
- レイヤー名は Cormorant Garamond セリフ体
- MOD / STATE チップは RING メイン画面と同一 LVGLオブジェクトを使い回す
- STATE チップは IME（あ）のみ。Caps Word 表示は両画面とも廃止
- AI 使用率を縦棒グラフ4本で表現
- バー色は使用率の値によらず各プロバイダーのブランドカラー固定
- バッテリー残量・LAST・KEYS は表示しない（RING メイン画面で確認）

---

## 1.5 実装（as-built, 2026-05-30）

本仕様は v8 時点の設計。実装では以下が確定値・確定挙動。**乖離点は本節を正とする**。

**切替トリガ**
- Main ↔ AI Usage は **切替キー長押し**（既定 F21 = keycode `112`、約 700ms）または **ディスプレイ長押しタッチ**（約 700ms、CST816S）。
- 判定は「押し続けて閾値に達した時点」で切替（離した瞬間ではない）。閾値 `RING_AI_USAGE_LONGPRESS_MS = 700`。
- CONFIG: `PROSPECTOR_RING_AI_USAGE` / `_TOGGLE_KEY` / `_TOGGLE_KEYCODE` / `_TOGGLE_TOUCH`。

**レイヤー名**
- cormorant_22 は生成せず **cormorant_30** を使用（フォント切替＋座標移動）。位置 `(18, 29)`、幅 `104`、左寄せ。

**共通オブジェクト（AI Usage 時の座標）**
- MOD チップ C/S/A/G 中心 `136 / 166 / 196 / 226`（間隔 30）、IME チップ中心 `256`。いずれも **縦中心 43**（レイヤー名の字面中心に整合）。
- **Caps Word チップは撤去せず、AI Usage 時のみ hidden**（Main は不変。設計の「両画面で廃止」からは変更）。
- **輝度（アイコン＋数値）は AI Usage 時 hidden**。
- キーボード名・Uptime は据え置き。

**AI Usage 固有オブジェクト**
- **水平区切り線は廃止**（上段が窮屈なため）。
- AI USAGE ラベル `(14, 62)`。
- 列ラベル 5H/7D：各バー中心 x、`y=78`。
- バー：幅 `52`、上端 `y=94`、最大高 `84`、角丸 `5`。x = `24 / 90 / 156 / 222`（中心 `50 / 116 / 182 / 248`）。リセット行確保のため最大高は 96→84 に縮小（実機で重なる場合は更に調整）。
- 充填：`h = round(used% × 84 / 100)`、`fill_y = 94 + (84 − h)`（1px 下限）。色は固定ブランドカラー（Claude `#CC7E5A` / Codex `#7B6FE8`）。数値・ツール名も同色。
- 使用率数値：`y=180`（各バー中心 x）。
- リセット残り時間：`y=200`（各バー中心 x、dim 色 `ring_color_text_sec()`、montserrat_12）。後述「データ状態」参照。
- ツール名：Claude は左2本（中心 50/116）の中央 `cx=83`、Codex は右2本（中心 182/248）の中央 `cx=215`、`y=218`。
- プロバイダ間の縦区切り線：`x=149`、`y=78`〜`218`。

**データ状態**
- `ERR` は **window 無効かつ error_present** のときのみ。valid 時は error より stale/estimated を優先（例 `72%*`）。
- **リセット残り時間**：各バー下に window のリセットまでの相対時間を表示。
  - 表示条件：`quota_source(bit4)=1` かつ 対象 window の valid フラグ=1 かつ `reset_unix != 0`。満たさなければ空表示（Codex local-history fallback 等）。
  - 計算：`remaining = reset_unix − updated_unix − (現在uptime − received_uptime)`（受信時 uptime は getter の `received_uptime_ms`。TIME_SYNC 非依存）。`REFRESH_PERIOD`(2s) ごとに更新＝カウントダウン。
  - 表記（残り時間で適応）：`≥1日 → "Xd Yh"` / `1時間〜1日未満 → "Xh Ym"`（d 省略）/ `1時間未満 → "Ym"`（h 省略）。例 `3d 4h` / `5h 30m` / `45m`。

**z-order**
- `common_root` への集約は導入せず、`ai_usage_root` 生成後に `lv_obj_move_background(ai_usage_root)` で共通オブジェクトを前面維持。

---

## 2. プロバイダー

| provider_id | 画面表示名 | バーカラー |
|---|---|---|
| `claude_code` | Claude | `#CC7E5A` |
| `codex` | Codex | `#7B6FE8` |

---

## 3. カラートークン

RING メイン画面と完全に共通。

### 3.1 ベースカラー

| トークン | Light | Dark |
|---|---|---|
| 背景 | `#FAFAFA` | `#22282C` |
| プライマリテキスト | `#22282C` | `#E9E7E1` |
| セカンダリテキスト | `#5F6A70` | `#929FA7` |
| オフテキスト | `#C6CDD1` | `#3A4248` |
| チップOFF枠 | `#D8DCDF` | `#3A4248` |
| 区切り線 | `#E2E5E8` | `#3A4248` |
| バー背景（track） | `#E2E5E8` | `#3A4248` |

### 3.2 アクセントカラー

| トークン | カラー | 用途 |
|---|---|---|
| accent | `#E89B5C` | MOD/STATE チップ ON・輝度ドット |
| claude | `#CC7E5A` | claude_code バー・数値・ツール名 |
| codex | `#7B6FE8` | codex バー・数値・ツール名 |

ライト・ダーク共通。

---

## 4. フォント

| 用途 | フォント |
|---|---|
| キーボード名 | lv_font_montserrat_14、weight 500 |
| Uptime | lv_font_montserrat_12 |
| レイヤー名 | Cormorant Garamond 22pt（AI Usage時）/ 36pt（Main時） |
| MOD チップ文字 | Sans-serif 14pt、ON は weight 500 |
| STATE チップ文字（あ） | Sans-serif 11pt |
| AI USAGE ラベル | Sans-serif 9pt、letter-spacing 2px |
| 列ラベル（5H/7D） | Sans-serif 8pt、letter-spacing 1px |
| バー数値 + サフィックス | Sans-serif 12pt、weight 500 |
| ツール名 | Sans-serif 8pt、letter-spacing 1px |
| 輝度数値 | Sans-serif 11pt、weight 500 |

---

## 5. 画面構成

```
y=  0  ┌──────────────────────────────────────┐
       │ LotusUni                  UP  2:34   │  ← キーボード名 / Uptime（共通）
       │                                      │
       │ Navigation     │ ●C ○S ○A ●G ○あ    │  ← レイヤー名（共通）│ MOD/STATE（共通）
y= 58  ├──────────────────────────────────────┤  ← 水平区切り線（AI Usage生成）
       │ AI USAGE                             │  ← ラベル（AI Usage生成）
       │    5H    7D       5H    7D            │  ← 列ラベル（AI Usage生成）
       │   ████   ██     ███   █████          │
       │   ████   ██     ███   █████          │  ← 縦棒グラフ（AI Usage生成）
       │   ████   ██     ███   █████          │
       │   72%   31%*    45%e  ERR            │  ← 使用率数値（AI Usage生成）
       │    Claude          Codex             │  ← ツール名（AI Usage生成）
y=240  │ ● 70%                               │  ← 輝度（共通）
       └──────────────────────────────────────┘
```

---

## 6. LVGLオブジェクト管理

### 6.1 common_root について

キーボード名・Uptime・MODチップ・STATEチップ・レイヤー名・輝度は
`common_root`（`lv_scr_act()` の直接の子）にまとめる。

`ai_usage_root` を後から生成すると LVGL の z-order 上で前面に来るため、
生成後に `lv_obj_move_foreground(common_root)` を呼んで共通オブジェクトを
前面に戻す。

### 6.2 共通オブジェクト（メイン画面と共有）

メイン画面で生成済みのオブジェクトを座標移動して使い回す。

| オブジェクト | Main座標 | AI Usage座標 | 変更内容 |
|---|---|---|---|
| キーボード名 | `(18, 11)` | `(18, 11)` | なし |
| Uptime | `(198, 10)` | `(198, 10)` | なし |
| 輝度ドット | 左下 | 左下 | なし |
| 輝度数値 | 左下 | 左下 | なし |
| MODチップ C | `cx=218, cy=58` r=12 | `cx=160, cy=40` r=12 | 座標移動 |
| MODチップ S | `cx=248, cy=58` r=12 | `cx=184, cy=40` r=12 | 座標移動 |
| MODチップ A | `cx=218, cy=88` r=12 | `cx=208, cy=40` r=12 | 座標移動 |
| MODチップ G | `cx=248, cy=88` r=12 | `cx=232, cy=40` r=12 | 座標移動 |
| STATEチップ あ（IME） | `cx=248, cy=114` r=10 | `cx=256, cy=40` r=10 | 座標移動 |
| レイヤー名 | リング中央 36pt | `(18, 50)` 22pt | 座標＋フォントサイズ変更 |

### 6.3 AI Usage固有オブジェクト（表示時生成・離脱時破棄）

`ai_usage_root` は画面全体を覆うコンテナとして生成する。
全スタイルを除去し、座標・サイズを画面に合わせる。

```c
ai_usage_root = lv_obj_create(lv_scr_act());
lv_obj_remove_style_all(ai_usage_root);
lv_obj_set_pos(ai_usage_root, 0, 0);
lv_obj_set_size(ai_usage_root, 280, 240);
```

座標はすべて画面全体基準（`ai_usage_root` の原点 = 画面左上）。

| オブジェクト | 変数名 | 座標 / 内容 |
|---|---|---|
| 水平区切り線 | `line_ai_header_horizontal` | `x=14〜266, y=58` |
| レイヤー名・チップ間の縦区切り線 | `line_ai_layer_chips_vertical` | `x=144, y=26〜54` |
| AI USAGE ラベル | `label_ai_usage` | `(14, 69)` |
| 列ラベル 5H（claude_code） | — | `cx=50, y=80` |
| 列ラベル 7D（claude_code） | — | `cx=116, y=80` |
| 列ラベル 5H（codex） | — | `cx=182, y=80` |
| 列ラベル 7D（codex） | — | `cx=248, y=80` |
| バー背景 ×4 | — | 各バー x, y=86, w=52, h=100 |
| バー充填 ×4 | — | 各プロバイダーブランドカラー |
| 使用率数値 ×4 | — | y=202、各バー中心 x |
| ツール名 Claude | — | `cx=72, y=218` |
| ツール名 Codex | — | `cx=214, y=218` |
| グループ区切り線 | `line_ai_group_vertical` | `x=149, y=78〜220` |

### 6.4 Main固有オブジェクト（AI Usage表示中は HIDDEN）

| オブジェクト | 変数名 |
|---|---|
| バッテリーリング lv_arc ×2〜3 | `arc_p1`, `arc_p2` |
| メイン縦区切り線 x=190 | `line_main_vertical` |
| LAST ラベル＋値 | `label_last` |
| KEYS ラベル＋値 | `label_keys` |

---

## 7. 切替フロー

### 7.1 Main → AI Usage

```c
void ring_show_ai_usage(void) {
    if (ai_usage_root != NULL) {
        return;  // 二重呼び出し防止
    }

    // 1. Main固有オブジェクトを非表示
    lv_obj_add_flag(arc_p1,             LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(arc_p2,             LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(line_main_vertical, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(label_last,         LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(label_keys,         LV_OBJ_FLAG_HIDDEN);

    // 2. 共通オブジェクトを AI Usage の座標に移動
    lv_obj_set_pos(chip_c,   160-12, 40-12);
    lv_obj_set_pos(chip_s,   184-12, 40-12);
    lv_obj_set_pos(chip_a,   208-12, 40-12);
    lv_obj_set_pos(chip_g,   232-12, 40-12);
    lv_obj_set_pos(chip_ime, 256-10, 40-10);
    lv_obj_set_pos(label_layer, 18, 50);
    lv_obj_set_style_text_font(label_layer, &cormorant_22, 0);

    // 3. AI Usage root を生成（全スタイル除去・画面全体に配置）
    ai_usage_root = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(ai_usage_root);
    lv_obj_set_pos(ai_usage_root, 0, 0);
    lv_obj_set_size(ai_usage_root, 280, 240);
    ring_ai_usage_build(ai_usage_root);

    // 4. common_root を前面に移動（ai_usage_root に共通オブジェクトが隠れないよう）
    lv_obj_move_foreground(common_root);
}
```

### 7.2 AI Usage → Main

```c
void ring_show_main(void) {
    if (ai_usage_root == NULL) {
        return;  // 二重呼び出し防止
    }

    // 1. AI Usage root を破棄
    lv_obj_del(ai_usage_root);
    ai_usage_root = NULL;

    // 2. 共通オブジェクトを Main の座標に戻す
    lv_obj_set_pos(chip_c,   218-12, 58-12);
    lv_obj_set_pos(chip_s,   248-12, 58-12);
    lv_obj_set_pos(chip_a,   218-12, 88-12);
    lv_obj_set_pos(chip_g,   248-12, 88-12);
    lv_obj_set_pos(chip_ime, 248-10, 114-10);
    lv_obj_set_pos(label_layer, /* リング中央座標 */);
    lv_obj_set_style_text_font(label_layer, &cormorant_36, 0);

    // 3. Main固有オブジェクトを再表示
    lv_obj_clear_flag(arc_p1,             LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(arc_p2,             LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(line_main_vertical, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(label_last,         LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(label_keys,         LV_OBJ_FLAG_HIDDEN);
}
```

### 7.3 切替トリガー

長押しキーコードによる切替（タップ誤操作防止のため長押し推奨）。

```conf
CONFIG_RING_AI_USAGE_TOGGLE_KEY=y
CONFIG_RING_AI_USAGE_TOGGLE_KEYCODE=<keycode>
```

---

## 8. AI Usage セクション詳細

### 8.1 バーグラフ

| 項目 | 値 |
|---|---|
| バー幅 | 52px |
| バー最大高 | 100px（y=86〜186） |
| 角丸 | rx=5 |

バー x 座標・中心 x：

| バー | x | 中心 x |
|---|---|---|
| claude_code 5H | 24 | 50 |
| claude_code 7D | 90 | 116 |
| codex 5H | 156 | 182 |
| codex 7D | 222 | 248 |

充填高さの計算：

```
fill_height = round(100 * usage_percent / 100)
fill_y      = 86 + (100 - fill_height)
```

### 8.2 データ状態表示

数値にサフィックスを付加してデータの鮮度・状態を示す。
フォント・カラーはバー数値と同じ。

複数の状態フラグが同時に立つ場合は以下の優先順位で表示を決定する：

```
error > stale > estimated > normal
```

| 優先 | 状態 | 表示例 | 条件 |
|---|---|---|---|
| 1 | `error` | `ERR` | エラーフラグあり。バー充填なし |
| 2 | `stale` | `72%*` | staleフラグあり（estimatedと同時でも `*` 優先） |
| 3 | `estimated` | `72%e` | estimatedフラグのみ |
| 4 | `valid` | `72%` | 正常 |
| — | `valid_off` | `--%` | データ提供が無効（意図的にオフ）。バー充填なし |

---

## 9. データソース

| 要素 | 取得元 |
|---|---|
| キーボード名 | `CONFIG_ZMK_KEYBOARD_NAME` |
| Uptime | ドングル起動からの経過時間 |
| レイヤー名 | `zmk_keymap_layer_name()` |
| MOD 状態 | `zmk_hid_get_explicit_mods()` |
| IME 状態 | `INTERNATIONAL4` で JP、`INTERNATIONAL5` で EN |
| 輝度 | `prospector_brightness_get_user_level()` |
| AI 使用率 | `zmk-rawhid-app` モジュールが RawHID で受信し `rawhid_app_ai_usage_get()` で供給 |

---

## 10. 現状の制限

- AI 使用率データはドングル外部からの連携が必要
- 5時間枠・7日枠のみ対応
- リセットは「残りまでの相対時間」を各バー下に表示（絶対時刻ではない）。表示条件は quota_source 等を満たす枠のみ
- バッテリー残量はこの画面では表示しない（RING メイン画面で確認）
- レイヤー名のフォントサイズ切替のため cormorant_22 と cormorant_36 の両方をメモリに保持する必要がある

---

## 改訂履歴

| 日付 | 内容 |
|---|---|
| 2026-05-15 | 初版作成 |
| 2026-05-15 | バー色の使用率による変化を廃止。プロバイダー定義・データ状態表示を追加 |
| 2026-05-15 | STATE チップから Caps Word（⇪）を削除しIMEのみに変更。MainとAI Usage両画面に適用 |
| 2026-05-15 | LVGLオブジェクト共有設計を採用。MOD/STATEチップ・レイヤー名・輝度をMainと共通化 |
| 2026-05-15 | common_rootへの集約とlv_obj_move_foreground追加。ai_usage_rootのstyle除去・フルサイズ配置を明記。区切り線の変数名を分離。二重呼び出しガード追加。データ状態の優先順位を定義 |
| 2026-05-30 | 実装に合わせ §1.5「実装(as-built)」を追加。cormorant_30採用（cormorant_22未生成）、横区切り線廃止、AI Usage時の輝度非表示、座標一式更新、切替=長押しキー(F21)/長押しタッチ、チップ縦中心揃え(43)、Caps Wordは撤去せずhidden、ERRは無効window+error時のみ、z-orderはmove_background採用 |
| 2026-05-31 | リセット残り時間の相対表示を追加（バー高 96→84、値 y180 / reset y200 / ツール名 y218）。表記は残り時間で適応（Xd Yh / Xh Ym / Ym）。ツール名を左右各2本の中央へ（cx 83 / 215）。getter に received_uptime_ms 追加（カウントダウン用）。 |
