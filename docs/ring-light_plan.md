# ring をメイン画面専用タッチ操作に整理する計画

## Summary
- `feat/ring-light` 上で、ring の bootloader / brightness 専用画面とページ遷移 UI を削除する。
- 表示画面はメイン画面のみとし、既存機能はメイン画面上のタッチ/ジェスチャーで直接実行する。
- bootloader は `↓` スワイプ後、短時間内の `→` スワイプで確認なしに入る連続スワイプ方式にする。
- brightness はメイン画面左半分タップで down、右半分タップで up にする。
- theme はメイン画面の下→上スワイプで light/dark を切り替える。
- `CONFIG_PROSPECTOR_RING_GESTURE_NAV` は既存 conf を壊さないため名前を残し、意味を「main-screen touch gestures」に更新する。

## Key Changes
- `ring_nav.c/.h`、`page_bootloader.c/.h`、`page_brightness.c/.h` を削除し、CMake からもビルド対象にしない。
- `status_screen.c` から gesture nav 用の main page wrapper、hidden page 初期化、page dots、bootloader/brightness page 初期化を削除し、全 widget を直接 screen に配置する。
- `ring_theme.c` から page/nav への theme reapply 呼び出しを削除する。
- `ring_touch.c` にメイン画面用タッチ操作を集約する。
  - `SWIPE_DOWN` を受けたら bootloader arm 状態にする。
  - arm 状態から一定時間内、推奨 `800ms`、に `SWIPE_RIGHT` を受けたら `bootmode_set(BOOT_MODE_TYPE_BOOTLOADER)` -> `sys_reboot(SYS_REBOOT_WARM)` を `lv_async_call` で実行する。
  - `SWIPE_UP` は `ring_theme_toggle()` を呼ぶ。
  - `SINGLE_CLICK` は画面座標に変換した `x` が `SCREEN_W / 2` 以上なら `prospector_brightness_adjust_user_level(CONFIG_PROSPECTOR_BRIGHTNESS_STEP)`、左ならマイナス方向に調整する。
  - brightness 操作は既存の `CONFIG_PROSPECTOR_BRIGHTNESS_STEP` を使う。
- `Kconfig` の説明を更新する。
  - `PROSPECTOR_RING_DARK_TOGGLE_TOUCH` は double tap ではなく swipe-up theme toggle へ説明を変更、または `PROSPECTOR_RING_GESTURE_NAV` 側に統合する。
  - `PROSPECTOR_RING_GESTURE_NAV` は Bootloader/Brightness pages ではなく main-screen touch gestures として説明する。
- CMake の touch source 条件は、既存設定互換のため `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH OR CONFIG_PROSPECTOR_RING_GESTURE_NAV` を維持する。ただし実運用では `CONFIG_PROSPECTOR_RING_GESTURE_NAV=y` が新しいメイン画面操作の有効化設定になる。

## Test Plan
- `rg` で `ring_nav`、`page_bootloader`、`page_brightness`、`Bootloader/Brightness pages`、page dots、hidden page など旧ページ UI 参照が残っていないことを確認する。
- `rg` で `PROSPECTOR_RING_GESTURE_NAV`、`PROSPECTOR_RING_DARK_TOGGLE_TOUCH`、`CONFIG_PROSPECTOR_BRIGHTNESS_STEP` が意図通り使われていることを確認する。
- `hitsuki46_dongle prospector_adapter` 構成で `west build` し、ビルド成功とメモリ使用量を旧ビルドと比較する。
- 実機確認項目:
  - メイン画面のみ表示され、左右スワイプで別画面へ遷移しない。
  - 右半分タップで輝度 up、左半分タップで輝度 down。
  - 下から上スワイプでテーマ切り替え。
  - 下スワイプ後 `800ms` 以内の右スワイプで bootloader に入る。
  - 単独の下/右/左スワイプでは bootloader に入らない。

## Assumptions
- bootloader 連続スワイプの受付時間は `800ms` を初期値にする。
- `CONFIG_PROSPECTOR_RING_GESTURE_NAV=y` は名前を変えずに残し、新しいメイン画面タッチ操作の有効化フラグとして扱う。
- `CONFIG_PROSPECTOR_RING_DARK_TOGGLE_TOUCH` も互換性のため残すが、今後の主設定は `PROSPECTOR_RING_GESTURE_NAV` とする。
- brightness 専用の表示フィードバックは追加しない。画面数と LVGL object 削減を優先する。
