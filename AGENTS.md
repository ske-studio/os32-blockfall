# AGENTS.md

## 優先ドキュメント
`docs/SPEC.md` > `docs/AI_TASK.md` > `docs/TEST_PLAN.md` > `README.md`

## ビルドと検証
- **前提**: `../os32` で `make sdk` が完了していること
- **ビルド**: `make` (SDKパスが異なる場合は `make OS32_SDK=/path/to/sdk`)
- **成果物**: `build/blockfall.bin`
- **検証**: `Makefile` の `-Wall` による警告なしでビルドすること

## 必須制約 (Breakout の教訓)
- **SDK**: 公開 API のみ使用。OS32 本体や KAPI を変更/想像して追加しない。
- **入力**: `kbd_trygetchar()` のリピートに依存せず、`kbd_is_pressed()` を使用する。
- **タイミング**: busy wait 禁止。`get_tick()` + `sys_halt()` で制御する。
- **終了**: 正常終了時は必ず `libos32gfx_shutdown()` を呼ぶ。
- **ツールチェーン**: Makefile で GCC バージョン等のパスをハードコードしない。

## 実装ルール
- **言語**: C (GNU89) / freestanding。C99 以降の構文禁止。
- **禁止事項**: 浮動小数点、動的メモリ確保、外部アセット(画像/音)の追加。
- **構造**: ゲーム状態は構造体に集約。盤面は `u8 board[20][10]` 固定配列。
- **描画順**: 背景 → 枠 → 固定ブロック → アクティブピース → ステータス → NEXT → オーバーレイ → `gfx_present()`

## 入力処理の責務
- **Held (左右移動, ソフトドロップ)**: `kbd_is_pressed()` を使用。左右移動はゲーム側で DAS/ARR タイマを実装し、OS のリピート速度に依存させない。
- **Edge (回転, ハードドロップ, リスタート, Esc)**: `current && !previous` で検出。押しっぱなしで連続動作させない。

## 変更範囲
- **許可**: `src/`, `docs/`, `Makefile` (ビルド修正のみ), `README.md`, `app.conf`
- **禁止**: OS32 本体の変更、HOLD/ゴーストピース/T-spin/BGM/セーブ/ネットワーク等の MVP 外機能の追加。

まず `docs/SPEC.md` の MVP を完遂させること。
