# Prospector ZMK Module — RING レイアウト

[Prospector](https://github.com/carrefinho/prospector) ディスプレイドングル向けカスタムステータス画面 ZMK モジュールです。
本モジュールはオリジナル Prospector ZMK Module のレイアウトの 1 つである **RING** 専用の実装です。

![RING レイアウト ライト/ダークテーマ](docs/images/mockup-dark.png)

> [!IMPORTANT]
> このブランチは開発中です。ZMK の Zephyr 4.1 ベースバージョン（現在の main）にのみ対応しています。

## 目次

- [特徴](#特徴)
- [インストール](#インストール)
- [ステータス画面](#ステータス画面)
- [使い方](#使い方)
- [設定](#設定)
- [トラブルシューティング](#トラブルシューティング)
- [既知の問題](#既知の問題)
- [To-Do](#to-do)
- [ライセンス](#ライセンス)

## 特徴

- 同心円バッテリーリング（ペリフェラル数 1〜3 に自動対応）
- レイヤー名表示
- ペリフェラルバッテリー残量表示
- BLE プロファイル番号 / USB 出力表示
- CTRL / SHFT / ALT / GUI モディファイアチップ（押下時ハイライト）
- Caps Word インジケーター
- IME 状態インジケーター（変換キーで ON、無変換キーで OFF）
- 打鍵カウンター
- ダブルタップでライト / ダークテーマ切り替え（CST816S タッチパネル搭載機）
- スワイプジェスチャーナビゲーション — 左から右で Bootloader 確認画面、右から左で輝度調整画面（CST816S タッチパネル搭載機、オプション）

## インストール

ZMK キーボードはドングルをセントラルとしてセットアップしてください。

`config/west.yml` の `remotes` と `projects` に以下を追加します:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: hrmt-lab                               # <--- 追加
      url-base: https://github.com/hrmt-lab        # <--- 追加
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: prospector-zmk-module                  # <--- 追加
      remote: hrmt-lab                             # <---
      revision: feat/new-status-screens            # <---
  self:
    path: config
```

次に `build.yaml` のドングルに `prospector_adapter` シールドを追加します:

```yaml
---
include:
  - board: xiao_ble//zmk
    shield: [YOUR_KEYBOARD_SHIELD]_dongle prospector_adapter
```

ZMK モジュールとローカルビルドの詳細は [ZMK ドキュメント（モジュール）](https://zmk.dev/docs/features/modules) を参照してください。

## ステータス画面

RING を有効にするには `.conf` ファイルに以下を追加します:

```ini
CONFIG_PROSPECTOR_STATUS_SCREEN_RING=y
```

### RING レイアウトの構成

**左パネル — バッテリーリング**

ペリフェラル数（`ZMK_SPLIT_BLE_PERIPHERAL_COUNT`）に応じてリングが自動調整されます:

- **1 ペリフェラル** — 幅広の 1 本リング、大きなレイヤー名
- **2 ペリフェラル** — 2 本の同心円リング（デフォルト）
- **3 ペリフェラル** — 3 本のリング、やや小さめのレイヤー名

**右パネル — 状態表示**

- ドングルバッテリー残量（BLE 接続時のみ）
- CTRL / SHFT / ALT / GUI モディファイアチップ
- CAPS / IME 状態チップ
- 出力エンドポイント（BLE プロファイル番号または USB）
- 打鍵カウンター

IME 状態は `INTERNATIONAL4`（変換）キーで ON、`INTERNATIONAL5`（無変換）キーで OFF になります。

## 使い方

**ペアリング順序**

スプリットキーボードでは、ドングルをフラッシュした後、左側のペリフェラルから先にペアリングしてください。ペリフェラルが 3 台以上ある場合は左から右の順でペアリングします。

**レイヤー名の設定**

レイヤー表示は `display-name` プロパティがあればそれを使用し、なければレイヤーインデックスを表示します。キーマップに `display-name` を追加するには:

```dts
keymap {
  compatible = "zmk,keymap";
  base {
    display-name = "Base";           # <--- 追加
    bindings = <
      ...
    >;
  }
}
```

**テーマ切り替え**

CST816S タッチパネルを搭載した Prospector ドングルでは、`CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH=y` を有効にすることで画面をダブルタップしてライトテーマとダークテーマを切り替えられます。

**ジェスチャーナビゲーション**

`CONFIG_PROSPECTOR_RING_GESTURE_NAV=y` を有効にすると、スワイプで追加画面に移動できます。ダブルタップによるテーマ切り替えとは独立して有効化できます:

| ジェスチャー | 動作 |
|---|---|
| 左から右へスワイプ | Bootloader 確認画面（Flash / Cancel） |
| Bootloader 画面で右から左へスワイプ | メイン画面へ戻る |
| 右から左へスワイプ | 輝度調整画面（左半分タップで -10%、右半分タップで +10%） |
| 輝度調整画面で左から右へスワイプ | メイン画面へ戻る |

画面下部のページドット（3点）で現在の画面位置を確認できます。

## 設定

`.conf` ファイルに設定を追加してカスタマイズできます:

```ini
CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR=n
CONFIG_PROSPECTOR_FIXED_BRIGHTNESS=80
CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT=30
CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL=y
```

### 共通設定

| 名前 | 説明 | デフォルト |
| ---- | --- | --------- |
| `CONFIG_PROSPECTOR_ROTATE_DISPLAY_180` | ディスプレイを 180 度回転 | n |
| `CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR` | 照度センサーで輝度を自動調整 | y |
| `CONFIG_PROSPECTOR_FIXED_BRIGHTNESS` | 照度センサー無効時の固定輝度 | 50 (1–100) |
| `CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT` | 無操作で画面をオフにするまでの秒数（`0` で無効） | 0 |
| `CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL` | キーコードで輝度を調整 | n |
| `CONFIG_PROSPECTOR_BRIGHTNESS_UP_KEYCODE` | 輝度アップのキーコード | 115 (F24) |
| `CONFIG_PROSPECTOR_BRIGHTNESS_DOWN_KEYCODE` | 輝度ダウンのキーコード | 114 (F23) |
| `CONFIG_PROSPECTOR_BRIGHTNESS_STEP` | 1 回のキー押下で変化する輝度 | 10 |
| `CONFIG_PROSPECTOR_LAYER_NAME_UPPERCASE` | レイヤー名を大文字に変換 | y |

輝度キー制御を有効にした場合は、設定したキーコードを送出するキーをキーマップに割り当ててください。デフォルトは F24（輝度アップ）/ F23（輝度ダウン）です。

### モディファイア設定

| 名前 | 説明 | デフォルト |
| ---- | --- | --------- |
| `CONFIG_PROSPECTOR_SHOW_MODIFIERS` | モディファイアインジケーターを表示 | y |

### RING ダークテーマ切り替え

| 名前 | 説明 | デフォルト |
| ---- | --- | --------- |
| `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH` | ディスプレイのダブルタップでライト/ダークテーマを切り替え（CST816S タッチコントローラー必須） | n |
| `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEY` | キーコードでライト/ダークテーマを切り替え | n |
| `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEYCODE` | テーマ切り替えキーコード（`DARK_TOGGLE_KEY` 有効時） | 111 (F20) |

### RING ジェスチャーナビゲーション

| 名前 | 説明 | デフォルト |
| ---- | --- | --------- |
| `CONFIG_PROSPECTOR_RING_GESTURE_NAV` | スワイプナビゲーションを有効化（CST816S タッチコントローラー必須。`DARK_TOGGLE_TOUCH` とは独立） | n |

## トラブルシューティング

### RAM オーバーフローエラー

ビルド時に `region 'RAM' overflowed` エラーが発生した場合は、`.conf` ファイルに以下を追加してディスプレイバッファサイズを削減してください:

```ini
CONFIG_LV_Z_VDB_SIZE=25
```

## 既知の問題

- ドングルへの接続後、一部のペリフェラルがキー入力を認識しないことがある。該当のペリフェラルをリセットしてください。[zmkfirmware/zmk#3156](https://github.com/zmkfirmware/zmk/issues/3156)
- バッテリー表示は最大 3 ペリフェラルまでの対応です。

## To-Do

- ライト / ダーク初期テーマの設定オプション
- レイヤー名スタイルのカスタマイズ

## ライセンス

MIT License

本プロジェクトは [carrefinho](https://github.com/carrefinho) による
[Prospector ZMK Module](https://github.com/carrefinho/prospector) を基に作成されています。
MIT ライセンスの条件に従い使用・改変・再配布することができます。

Copyright (c) 2024 carrefinho

---

# Prospector ZMK Module — RING Layout

This is a [ZMK module](https://zmk.dev/docs/features/modules) that provides the **RING** custom status screen layout for the [Prospector](https://github.com/carrefinho/prospector) display dongle.
RING is one of the original layouts from the Prospector ZMK Module by carrefinho.

![RING layout light/dark theme](docs/images/mockup-dark.png)

> [!IMPORTANT]
> This branch is a work-in-progress and is only compatible with the Zephyr 4.1 version of ZMK (current main).

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Status Screen](#status-screen)
- [Usage](#usage)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Known Issues](#known-issues)
- [To-Do](#to-do-1)
- [License](#license)

## Features

- Concentric battery arc rings (auto-adapts to 1–3 peripherals)
- Active layer name display
- Peripheral battery status
- BLE profile number / USB output indicator
- CTRL / SHFT / ALT / GUI modifier chips (highlight when pressed)
- Caps Word indicator
- IME state indicator (INTERNATIONAL4 = ON, INTERNATIONAL5 = OFF)
- Keystroke counter
- Double-tap to toggle light / dark theme (CST816S touch panel)
- Swipe gesture navigation — left-to-right for Bootloader confirmation, right-to-left for Brightness adjustment (CST816S touch panel, optional)

## Installation

Your ZMK keyboard should be set up with a dongle as central.

Add this module to your `config/west.yml` under `remotes` and `projects`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: hrmt-lab                               # <--- add this
      url-base: https://github.com/hrmt-lab        # <--- and this
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: prospector-zmk-module                  # <--- and these
      remote: hrmt-lab                             # <---
      revision: feat/new-status-screens            # <---
  self:
    path: config
```

Then add the `prospector_adapter` shield to the dongle in your `build.yaml`:

```yaml
---
include:
  - board: xiao_ble//zmk
    shield: [YOUR_KEYBOARD_SHIELD]_dongle prospector_adapter
```

For more information on ZMK Modules and building locally, see [the ZMK docs page on modules](https://zmk.dev/docs/features/modules).

## Status Screen

Enable RING by adding the following to your `.conf` file:

```ini
CONFIG_PROSPECTOR_STATUS_SCREEN_RING=y
```

### RING Layout Structure

**Left panel — Battery rings**

The ring count and sizing adapt automatically to the peripheral count (`ZMK_SPLIT_BLE_PERIPHERAL_COUNT`):

- **1 peripheral** — single wide arc, large layer name
- **2 peripherals** — two concentric arcs (default)
- **3 peripherals** — three tighter arcs, slightly smaller layer name

**Right panel — Status indicators**

- Dongle battery level (BLE only)
- CTRL / SHFT / ALT / GUI modifier chips
- CAPS / IME state chips
- Active output endpoint (BLE profile number or USB)
- Keystroke counter

IME state is inferred from key events: `INTERNATIONAL4` (変換) sets IME on, `INTERNATIONAL5` (無変換) sets IME off.

## Usage

**Pairing order**

For split keyboards, pair the left peripheral first after flashing the dongle, then the right side. For more than two peripherals, pair them left to right.

**Layer display name**

The layer display shows the `display-name` property when available, falling back to the layer index otherwise. To add a `display-name` to a keymap layer:

```dts
keymap {
  compatible = "zmk,keymap";
  base {
    display-name = "Base";           # <--- add this
    bindings = <
      ...
    >;
  }
}
```

**Theme toggle**

On Prospector dongles with a CST816S touch panel, enable `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH=y` to toggle between light and dark themes by double-tapping the display.

**Gesture navigation**

Enable `CONFIG_PROSPECTOR_RING_GESTURE_NAV=y` to navigate between screens with swipe gestures. This can be enabled independently from double-tap theme switching:

| Gesture | Action |
|---|---|
| Swipe left-to-right | Bootloader confirmation screen (Flash / Cancel) |
| Swipe right-to-left on Bootloader | Return to Main |
| Swipe right-to-left | Brightness adjustment screen (tap left half −10%, right half +10%) |
| Swipe left-to-right on Brightness | Return to Main |

Three page-indicator dots at the bottom of the display show the current screen.

## Configuration

Customize by adding config options to your `.conf` file:

```ini
CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR=n
CONFIG_PROSPECTOR_FIXED_BRIGHTNESS=80
CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT=30
CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL=y
```

### General

| Name | Description | Default |
| ---- | ----------- | ------- |
| `CONFIG_PROSPECTOR_ROTATE_DISPLAY_180` | Rotate the display 180 degrees | n |
| `CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR` | Use ambient light sensor for auto brightness | y |
| `CONFIG_PROSPECTOR_FIXED_BRIGHTNESS` | Fixed display brightness when not using ambient light sensor | 50 (1–100) |
| `CONFIG_PROSPECTOR_DISPLAY_IDLE_TIMEOUT` | Seconds of inactivity before turning the display and backlight off (`0` disables) | 0 |
| `CONFIG_PROSPECTOR_BRIGHTNESS_KEY_CONTROL` | Control display brightness with keycodes | n |
| `CONFIG_PROSPECTOR_BRIGHTNESS_UP_KEYCODE` | Keycode for increasing display brightness | 115 (F24) |
| `CONFIG_PROSPECTOR_BRIGHTNESS_DOWN_KEYCODE` | Keycode for decreasing display brightness | 114 (F23) |
| `CONFIG_PROSPECTOR_BRIGHTNESS_STEP` | Brightness adjustment per key press | 10 |
| `CONFIG_PROSPECTOR_LAYER_NAME_UPPERCASE` | Convert layer names to uppercase | y |

When brightness key control is enabled, assign keys that emit the configured keycodes in your keyboard keymap. The defaults follow YADS/dongle-screen: F24 increases brightness and F23 decreases brightness.

### Modifiers

| Name | Description | Default |
| ---- | ----------- | ------- |
| `CONFIG_PROSPECTOR_SHOW_MODIFIERS` | Display modifier key indicators | y |

### RING Dark Theme Toggle

| Name | Description | Default |
| ---- | ----------- | ------- |
| `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH` | Toggle light/dark theme by double-tapping the display (requires CST816S touch controller) | n |
| `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEY` | Toggle light/dark theme via keycode | n |
| `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_KEYCODE` | Keycode for toggling theme (when `DARK_TOGGLE_KEY` is enabled) | 111 (F20) |

### RING Gesture Navigation

| Name | Description | Default |
| ---- | ----------- | ------- |
| `CONFIG_PROSPECTOR_RING_GESTURE_NAV` | Enable swipe navigation (requires CST816S touch controller; independent from `DARK_TOGGLE_TOUCH`) | n |

## Troubleshooting

### RAM overflow error

If you encounter a `region 'RAM' overflowed` error when building, add the following to your `.conf` file to reduce the display buffer size:

```ini
CONFIG_LV_Z_VDB_SIZE=25
```

## Known Issues

- One peripheral may fail to register key presses after connecting to the dongle; reset the affected peripheral to fix. [zmkfirmware/zmk#3156](https://github.com/zmkfirmware/zmk/issues/3156)
- Battery display supports up to three peripherals.

## To-Do

- Config option for default light / dark theme on boot
- Layer name style customization

## License

MIT License

This project is based on the [Prospector ZMK Module](https://github.com/carrefinho/prospector) by [carrefinho](https://github.com/carrefinho), licensed under the MIT License.

Copyright (c) 2024 carrefinho
