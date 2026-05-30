# RING レイアウト仕様書

この文書は、現在の実装を基にした Prospector Adapter 向け RING
レイアウトの仕様書です。

## 概要

RING は Prospector ディスプレイドングル向けのステータス画面
レイアウトです。280 x 240 の横長画面を前提に、分割キーボードの
状態を同心円状のバッテリーリング、修飾キー表示、状態チップ、
キー入力情報、メイン画面上のタッチ操作で表示/制御します。

有効化する CONFIG:

```conf
CONFIG_PROSPECTOR_STATUS_SCREEN_RING=y
```

## 画面前提

- 画面サイズは 280 x 240 の横長前提。
- 左側をメイン表示領域、右側をサイド情報領域として使う。
- 縦区切り線は x=190、y=36 から y=208 まで。
- 背景、文字色、補助色は RING テーマ経由で決定する。

## メイン画面

### キーボード名

- 表示内容は `CONFIG_ZMK_KEYBOARD_NAME`。
- 未定義の場合は `PROSPECTOR` を表示する。
- 位置は `(18, 11)`。
- フォントは `lv_font_montserrat_14`。
- 色は `ring_color_text_pri()`。

### Uptime 表示

右上にドングル起動後の経過時間を表示する。

- 位置は `(198, 10)`。
- 幅は `64`。
- 更新間隔は 60 秒固定。
- フォントは `lv_font_montserrat_12`。
- 色は `ring_color_text_off()`。
- 100 時間未満は `UP  h:mm` 形式。
- 100 時間以上は `UP  Nh` 形式。
- ルート画面に配置するため、サブ画面表示中も見える。

### バッテリーリング

接続されたペリフェラルのバッテリー残量を同心円のリングで表示する。

- 中心座標は `(96, 124)`。
- リング数は `ZMK_SPLIT_BLE_PERIPHERAL_COUNT` に従う。
- レイアウト上は最大 3 ペリフェラルまでを想定する。

ペリフェラル数ごとのリング仕様:

| ペリフェラル数 | 半径 | 線幅 | レイヤー名フォント |
| --- | --- | --- | --- |
| 1 | `70` | `5` | `CormorantGaramond_Regular_36` |
| 2 | `70, 55` | `4` | `CormorantGaramond_Regular_36` |
| 3 以上 | `70, 57, 44` | `4` | `CormorantGaramond_Regular_30` |

リング色:

| 対象 | 色 |
| --- | --- |
| Peripheral 1 | `RING_COLOR_P1` (`0x7A8B92`) |
| Peripheral 2 | `RING_COLOR_P2` (`0xA6B2B8`) |
| Peripheral 3 | `RING_COLOR_P3` (`0xC4CCD1`) |

動作:

- 接続中のペリフェラルはバッテリー残量をリングの塗り量で表す。
- 未接続時はリングの値を 0 にし、`ring_color_track()` で表示する。
- 未接続時のバッテリー数値は `-`。
- バッテリー残量は `zmk_peripheral_battery_state_changed` で更新する。
- 接続状態は `zmk_split_central_status_changed` で更新する。

### レイヤー名

リング中央に現在のアクティブレイヤー名を表示する。

- レイヤー名は `zmk_keymap_layer_name()` から取得する。
- レイヤー名が取得できない場合は `L<N>` を表示する。
- `CONFIG_PROSPECTOR_LAYER_NAME_UPPERCASE=y` の場合は大文字に変換する。
- 色は `ring_color_text_pri()`。

### 輝度表示

メイン画面左下に現在のユーザー輝度を表示する。

- 位置はおおよそ `(18, 210)`。
- 小さな丸い輝度アイコンと `NN%` ラベルを横並びで表示する。
- 現在値は `prospector_brightness_get_user_level()` から取得する。
- ラベル色は `ring_color_text_sec()`。
- アイコンは `RING_COLOR_ACCENT`。
- 左右タップで輝度を変更した直後に表示を更新する。

## サイド情報領域

サイド情報領域は x=206 から始まり、幅 64 px を基準に配置する。

### 修飾キー チップ

修飾キーの押下状態を丸いチップで表示する。

| 表示 | 意味 | 位置 |
| --- | --- | --- |
| `C` | Control | `(206, 46)` |
| `S` | Shift | `(236, 46)` |
| `A` | Alt | `(206, 76)` |
| `G` | GUI | `(236, 76)` |

動作:

- 押下中は `RING_COLOR_ACCENT` で塗りつぶす。
- 非押下時は透明背景、`ring_color_text_off()` の枠線と文字で表示する。
- 修飾キー状態は `zmk_hid_get_explicit_mods()` から取得する。

### 状態 チップ

修飾キー チップ の下に状態表示用の チップ を表示する。

| 表示 | 意味 | 位置 |
| --- | --- | --- |
| `⇪` | Caps Word | `(208, 114)` |
| `あ` | IME 状態 | `(238, 114)` |

動作:

- Caps Word は `CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED` が有効で、
  Caps Word が active のときに点灯する。
- IME は `INTERNATIONAL4` が入力されたら ON。
- IME は `INTERNATIONAL5` が入力されたら OFF。

### LAST 表示

最後に発行された HID キーを表示する。

有効化 CONFIG:

```conf
CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY=y
```

この CONFIG はデフォルトで有効。

配置:

- ヘッダー `LAST`: y=146。
- 値: y=160。
- 幅: `64`。
- 初期値: `---`。

動作:

- HID キー press イベントで更新する。
- 既知のキーコードは `A`, `ENTER`, `BSPC`, `TAB`, `F1`, `INT4`,
  `LCTL` などの短い名前で表示する。
- 未定義キーは `0x68` のような 16 進表記で表示する。
- 表示領域に合わせるため、キー名は短い表記を使う。

### KEYS 表示

キー押下回数をカウントして表示する。

配置:

- ヘッダー `KEYS`: y=184。
- 値: y=200。
- 幅: `64`。

動作:

- HID keyboard key の press イベントをカウントする。
- HID modifier keycode `0xE0` から `0xE7` はカウントしない。
- 表示形式:
  - `0` から `999`。
  - `1,000` から `99,999`。
  - それ以上は `99k+`。

## メイン画面タッチ操作

メイン画面タッチ操作は任意機能。

```conf
CONFIG_PROSPECTOR_RING_GESTURE_NAV=y
```

### スワイプ/タップ動作

この表のスワイプ方向は、画面上で見える指の移動方向を表す。

| 操作 | 動作 |
| --- | --- |
| 下スワイプ後、800 ms 以内に右スワイプ | Bootloader に入る |
| 画面左半分をタップ | 輝度を `CONFIG_PROSPECTOR_BRIGHTNESS_STEP` 下げる |
| 画面右半分をタップ | 輝度を `CONFIG_PROSPECTOR_BRIGHTNESS_STEP` 上げる |

補足:

- Bootloader は確認画面を持たず、条件成立時に即時 `BOOT_MODE_TYPE_BOOTLOADER`
  をセットして warm reboot する。
- 単独の下スワイプ、単独の右スワイプ、左スワイプでは Bootloader に入らない。
- 輝度変更後は左下の輝度パーセント表示を更新する。

### タッチ座標と向き

タッチコントローラーは CST816S を前提とする。

実装では CST816S のジェスチャーイベントを受け取るため、初期化時に
以下のレジスタを設定する。

- `MOTION_MASK` (`0xEC`)。
- `IRQ_CTL` (`0xFA`)。

現在の座標変換前提:

- 画面サイズは 280 x 240。
- デフォルト表示向きは `DISPLAY_ORIENTATION_ROTATED_270`。
- `CONFIG_PROSPECTOR_ROTATE_DISPLAY_180=y` の場合は、現在の rotated-90
  表示経路に合わせた変換を行う。

現時点では、0/90/180/270 度すべてを抽象化した汎用回転対応ではない。
現在の実装は、現行のデフォルト向きと既存の 180 度回転 CONFIG 経路を
対象にしている。

### 輝度 API

- 現在値取得: `prospector_brightness_get_user_level()`。
- 値変更: `prospector_brightness_adjust_user_level()`。

## テーマ

RING は実行時のライト/ダークテーマ切り替えに対応する。

初期状態:

- ライトテーマ。

キー操作による切り替え:

```conf
CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEY=y
CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEYCODE=111
```

タッチ操作による切り替え:

```conf
CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH=y
```

動作:

- キー切り替えは、設定された HID キーコードが press されたときに行う。
- デフォルトキーコードは `111`、つまり `F20`。
- タッチ切り替えは CST816S のダブルタップを使う。
- テーマ変更は `lv_async_call()` 経由で LVGL スレッド上にスケジュールする。
- テーマ変更後、RING の各ウィジェットに対して色を再適用する。

テーマ依存色:

| トークン | Light | Dark |
| --- | --- | --- |
| Background | `0xFAFAFA` | `0x22282C` |
| Primary text | `0x22282C` | `0xE9E7E1` |
| Secondary text | `0x5F6A70` | `0x929FA7` |
| Off text | `0xC6CDD1` | `0x3A4248` |
| Track | `0xE2E5E8` | `0x3A4248` |
| Inactive page dot | `0xD0D5D8` | `0x4A555C` |
| Cancel text | `0x929FA7` | `0x5F6A70` |
| Subtext | `0xC8CDD0` | `0x4A555C` |

テーマ非依存色:

| トークン | 値 |
| --- | --- |
| Accent | `0xE89B5C` |
| Peripheral 1 | `0x7A8B92` |
| Peripheral 2 | `0xA6B2B8` |
| Peripheral 3 | `0xC4CCD1` |

## CONFIG 一覧

| CONFIG | デフォルト | 内容 |
| --- | --- | --- |
| `PROSPECTOR_STATUS_SCREEN_RING` | `n` | RING レイアウトを選択する。 |
| `PROSPECTOR_RING_GESTURE_NAV` | `n` | メイン画面上のタッチ操作を有効化する。 |
| `PROSPECTOR_RING_DARK_TOGGLE_TOUCH` | `n` | ダブルタップでテーマ切り替えを有効化する。 |
| `PROSPECTOR_RING_DARK_TOGGLE_KEY` | `n` | キーコードでテーマ切り替えを有効化する。 |
| `PROSPECTOR_RING_DARK_TOGGLE_KEYCODE` | `111` | テーマ切り替え用 HID キーコード。 |
| `PROSPECTOR_RING_LAST_KEY_DISPLAY` | `y` | `LAST` 表示を有効化する。 |
| `PROSPECTOR_LAYER_NAME_UPPERCASE` | `y` | レイヤー名を大文字表示にする。 |
| `PROSPECTOR_ROTATE_DISPLAY_180` | `n` | 既存の 180 度回転表示経路を使う。 |
| `PROSPECTOR_BRIGHTNESS_KEY_CONTROL` | `n` | キーコードで輝度調整を有効化する。 |
| `PROSPECTOR_BRIGHTNESS_UP_KEYCODE` | `115` | 輝度アップ用キーコード。デフォルトは F24。 |
| `PROSPECTOR_BRIGHTNESS_DOWN_KEYCODE` | `114` | 輝度ダウン用キーコード。デフォルトは F23。 |
| `PROSPECTOR_BRIGHTNESS_STEP` | `10` | キー操作/タッチ操作時の輝度変更量。 |
| `PROSPECTOR_DISPLAY_IDLE_TIMEOUT` | `0` | 表示/バックライトを消灯するまでの秒数。0 は無効。 |
| `PROSPECTOR_RING_AI_USAGE` | `n` | AI Usage 画面を有効化する。 |
| `PROSPECTOR_RING_AI_USAGE_TOGGLE_KEY` | `n` | キーコード長押しで Main↔AI Usage 切替を有効化する。 |
| `PROSPECTOR_RING_AI_USAGE_TOGGLE_KEYCODE` | `112` | AI Usage 切替用 HID キーコード。デフォルトは F21。 |
| `PROSPECTOR_RING_AI_USAGE_TOGGLE_TOUCH` | `n` | 画面長押しタッチで Main↔AI Usage 切替を有効化する。 |

タッチ関連 CONFIG は、CST816S 入力に必要な CONFIG を自動で select する。

## ソース構成

RING 関連の主なファイル:

| ファイル | 役割 |
| --- | --- |
| `status_screen.c` | RING 画面を作成し、各ウィジェットを初期化する。 |
| `battery_rings.c` | ペリフェラルのバッテリーリングとレイヤー名。 |
| `brightness_info.c` | 左下の輝度アイコンと輝度パーセント表示。 |
| `modifier_chips.c` | 修飾キー チップと状態チップ。 |
| `keys_info.c` | `LAST` 表示と `KEYS` カウンター。 |
| `uptime_info.c` | 右上の uptime 表示。 |
| `ring_theme.c` | テーマ状態、切り替え、テーマ再適用。 |
| `ring_touch.c` | CST816S のジェスチャー設定とメイン画面上のタッチ操作、長押しタッチ切替。 |
| `ai_usage.c` | AI Usage 画面の生成/破棄、バー描画、長押しキー切替。 |
| `output_info.c` | 旧 Output 表示。現行 RING ビルドからは除外。 |

ビルド条件:

- `ring_touch.c` は、タッチテーマ切り替え・ジェスチャーナビゲーション・
  AI Usage タッチ切替のいずれかが有効な場合だけビルド対象になる。
- `output_info.c` は現行 RING ビルドから意図的に除外している。
  現在の RING では `KEYS`/`LAST` は `keys_info.c` にあり、
  USB/BLE 出力先表示とドングル側バッテリー表示は行わない。

## AI Usage 画面

`CONFIG_PROSPECTOR_RING_AI_USAGE=y` で有効化する、Main と切り替え可能な追加画面。
Claude / Codex の 5h・7d 使用率を縦棒グラフ4本で表示する。使用率データはキーボード側の
RawHID ハンドラ（例: hitsuki46 の `CONFIG_HITSUKI46_RAW_HID_AI_USAGE`）が getter 経由で供給する。
詳細レイアウトは [ring-ai-usage-ui-spec.md](ring-ai-usage-ui-spec.md) を参照。

切替:

- 切替キー長押し（既定 F21=`112`、約 700ms、`PROSPECTOR_RING_AI_USAGE_TOGGLE_KEY`）。
- 画面長押しタッチ（約 700ms、`PROSPECTOR_RING_AI_USAGE_TOGGLE_TOUCH`、CST816S）。
- いずれも「押し続けて閾値到達時」に切替（離した瞬間ではない）。`lv_async_call()` で LVGL スレッド上で実行。

レイアウト切替（Main → AI Usage）:

- 共通オブジェクト（MOD/IME チップ・レイヤー名）を AI Usage 座標へ移動。レイヤー名は cormorant_30 へ切替。
- Main 固有オブジェクト（バッテリーリング・`LAST`・`KEYS`・メイン縦区切り線・輝度・Caps Word チップ）を hidden。
- `ai_usage_root` を生成し、`lv_obj_move_background()` で共通オブジェクトを前面維持。
- 戻り（AI Usage → Main）で `ai_usage_root` を破棄し、座標・表示を復元。

更新:

- 表示中のみ `k_work_delayable`（`zmk_display_work_q()`）で getter をポーリングしてバー/数値を更新。
- テーマ切替時は `ring_ai_usage_apply_theme()` で背景/track/ラベル色を再適用。

## 現状の制限と注意点

- 現在の配置は 280 x 240 横長画面に最適化されている。
- 0/90/180/270 度すべてを統一的に扱う汎用回転機能は未実装。
- Bootloader と Brightness は専用画面を持たず、メイン画面上のタッチ操作で実行する。
- USB/BLE の出力先表示とドングル側バッテリー表示は現行 RING 画面にはない。
- `output_info.c` は旧実装/参考用としてソースツリーに残している。
- `LAST` 表示は軽量なフィードバック/デバッグ用途であり、
  `CONFIG_PROSPECTOR_RING_LAST_KEY_DISPLAY=n` で無効化できる。
