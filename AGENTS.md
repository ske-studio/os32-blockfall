# AGENTS.md

このリポジトリは、AI コーディングエージェントに OS32 向け落ち物パズルゲームを実装させるためのプロジェクトです。

## 最初に読むもの

1. `README.md`
2. `docs/SPEC.md`
3. `docs/AI_TASK.md`
4. `docs/TEST_PLAN.md`

仕様の優先順位は `docs/SPEC.md` > その他の文書 > 実装上の推測です。

## Breakout から引き継ぐ重要事項

- OS32 本体を変更しない。
- OS32 SDK の公開ヘッダ/API だけを使う。
- 未確認の KAPI を想像して使わない。
- `kbd_trygetchar()` のキーリピートにゲーム操作を依存させない。
- held input は `kbd_is_pressed()` を使う。
- 回転やハードドロップは edge 検出 (`current && !previous`) にする。
- busy wait しない。`get_tick()` + `sys_halt()` を使う。
- 終了時は必ず `libos32gfx_shutdown()` を呼ぶ。
- Makefile はクロスコンパイラ/GCCバージョンを固定値にしない。

## 実装ルール

- C 言語 / GNU89 / freestanding。
- C99 以降専用構文へ依存しない。
- 外部ライブラリ・外部画像・外部フォント・外部音声を追加しない。
- MVP に不要な機能を追加しない。
- ゲーム状態は構造体にまとめる。
- 盤面は `u8 board[20][10]` 相当の固定配列を基本とする。
- ピース定義は固定テーブルでよい。動的確保は不要。
- 浮動小数点は使わない。
- 画面更新順を固定する。

```text
input sample
 -> edge input
 -> held/repeat input
 -> gravity
 -> movement/rotation validation
 -> lock
 -> line clear
 -> spawn / game-over check
 -> render
 -> wait next tick
```

## 入力の責務

### held

- Left
- Right
- Soft drop

左右は押した瞬間に 1 cell 動かし、その後ゲーム側の DAS/ARR 相当のタイマで繰り返す。OS 側キーボードリピートは使わない。

### edge

- Rotate CW
- Rotate CCW (MVP で採用する場合のみ)
- Hard drop
- Restart

押しっぱなしで 1 tick ごとに回転/落下しないこと。

## 描画

- `libos32gfx` を使用する。
- MVP では全画面再描画でもよいが、ゲームロジックを present 回数に依存させない。
- OS32 640×400 を前提とするが、盤面の論理座標と画面座標は分離する。
- プレイ領域は 240×320 px、盤面は 160×320 px (10×20, 16px/cell)。

## AI が変更してよい範囲

- `src/`
- `docs/`（実装事実との同期）
- `Makefile`（ビルド修正のみ）
- `README.md`（操作/ビルド/実装状況の同期）
- `app.conf`

## AI が勝手に行わないこと

- OS32 / os32-game / os32-apps の変更
- KAPI の追加
- HOLD の追加
- ゴーストピースの追加
- T-spin 等の高度なスコアリング
- BGM/効果音
- ハイスコア保存
- ネットワーク
- マウス操作
- 外部アセット
- 一般ゲームエンジン化

まず `docs/SPEC.md` の MVP を完成させること。
