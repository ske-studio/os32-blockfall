# OS32 Blockfall

OS32 向けの落ち物パズルゲームです。`os32-breakout` で得たビルド・入力・フレーム制御の知見を最初から反映し、OS32 SDK の公開 API だけで実装します。

## MVP

- [x] 10 列 × 20 行の盤面
- [x] 7 種類の 4 ブロックピース（I/O/T/S/Z/J/L）
- [x] 左右移動 / 回転 / ソフトドロップ / ハードドロップ
- [x] 接地・固定・ライン消去（1-4 行同時）
- [x] SCORE / LINES / LEVEL / NEXT
- [x] GAME OVER / Restart
- [x] Esc で終了
- [x] ビルド完了

HOLD、ゴースト、BGM/効果音、セーブ、ランキング、ネットワークは MVP に含めません。

## 実装状況

### 完了
- ゲームロジック（game.c / game.h）
  - ピース生成（7-bag 消費方式、NEXT は次回 spawn と一致）
  - 衝突判定（回転済みセルテーブルを描画と共通使用）
  - 移動・回転（SPEC§5 の wall kick 順序、O は回転不要）
  - 降下不能で固定 → ライン消去 → スコア → 次ピース spawn
  - スコア計算 / NEXT プレビュー
- 入力処理（input.c / input.h）
  - held input（`kbd_is_pressed()`: 左右・ソフトドロップ）
  - edge detection（前 tick との比較: 回転、ハードドロップ、リスタート、Esc）
  - DAS/ARR ゲーム側制御（DAS=15, ARR=4）
- 描画（game.c）
  - プレイエリア・ステータスパネル
  - 盤面・ピース・ネクスト表示
  - READY/GAME OVER オーバーレイ

### リファクタリング済み
- 型定義を common.h に一元化
- ピース形状を 4×4 ローカル座標の回転済みセルテーブルに再設計
- レイアウト定数を game.h に一元化（セル数とピクセルの分離）
- 描画を draw_board / draw_active / draw_status / draw_next / draw_overlay に分割

## 画面

OS32 の 640×400 画面を使用します。

```text
640x400

   PLAY AREA 240x320              STATUS
  +----------------------+       +----------------------+
  |    +----------+      |       | BLOCKFALL            |
  |    | 10 x 20  |      |       |                      |
  |    |          |      |       | SCORE                |
  |    |          |      |       | 000000               |
  |    |          |      |       |                      |
  |    |          |      |       | LINES                |
  |    |          |      |       | 000                  |
  |    |          |      |       |                      |
  |    |          |      |       | LEVEL                |
  |    |          |      |       | 01                   |
  |    |          |      |       |                      |
  |    +----------+      |       | NEXT                 |
  +----------------------+       +----------------------+
```

## 入力方針

OS32 Breakout で文字入力の OS リピート依存が問題になったため、本ゲームは key state と edge detection を基本とする。入力は全て `kbd_is_pressed()` から取得する。

- Left / A     move left
- Right / D    move right
- Down / S     soft drop
- Up / X       rotate clockwise
- Z            rotate counter-clockwise
- Space        hard drop / READY start
- R            restart on GAME_OVER
- Esc          exit

- 左右・ソフトドロップ: held state + ゲーム側リピート（DAS=15, ARR=4）
- 回転・ハードドロップ・リスタート・Esc: 前 tick との差分で edge 検出
- OS 側のキーボードリピート速度にゲーム性を依存させない

## Build

OS32 SDK を先に生成してください。

```sh
cd ../os32
make sdk

cd ../os32-blockfall
make
```

SDK の場所が異なる場合:

```sh
make OS32_SDK=/path/to/os32/build/sdk
```

生成物:

```text
build/blockfall.bin
```

詳細は `docs/SPEC.md`, `docs/AI_TASK.md`, `docs/TEST_PLAN.md` を参照してください。

## 時間管理

ゲームループは OS32 の `get_tick()` と `sys_halt()` を使います。

- 入力: 毎 tick
- 自然落下: gravity timer で独立管理
- 描画: 状態更新後
- busy wait はしない
