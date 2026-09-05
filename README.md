# OS32 Blockfall

OS32 向けの落ち物パズルゲームです。`os32-breakout` で得たビルド・入力・フレーム制御の知見を最初から反映し、OS32 SDK の公開 API だけで実装します。

## MVP

- 10 列 × 20 行の盤面
- 7 種類の 4 ブロックピース
- 左右移動 / 回転 / ソフトドロップ / ハードドロップ
- 接地・固定・ライン消去
- SCORE / LINES / LEVEL / NEXT
- GAME OVER / Restart
- `Esc` で終了

HOLD、ゴースト、BGM/効果音、セーブ、ランキング、ネットワークは MVP に含めません。

## 画面

OS32 の 640×400 画面を使用します。

- 左側のプレイ領域: **240×320 px**
- 盤面: **10×20 cells**, 1 cell = **16×16 px** → 160×320 px
- プレイ領域内の左右 40 px は余白/枠として使用
- 右側に SCORE / LINES / LEVEL / NEXT を表示

```text
640x400

   PLAY AREA 240x320              STATUS
  +----------------------+       +----------------------+
  |    +----------+      |       | BLOCKFALL            |
  |    | 10 x 20  |      |       |                      |
  |    |          |      |       | SCORE                |
  |    | 16x16 px |      |       | 000000               |
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

Breakout で `kbd_trygetchar()` のキーリピート依存が問題になったため、Blockfall では `kbd_is_pressed()` を前提にします。

- 左右・ソフトドロップ: held state + ゲーム側リピート
- 回転・ハードドロップ: 前フレームとの差分で edge 検出
- `kbd_trygetchar()` は `Esc` など文字入力補助に使用可能

OS 側のキーボードリピート速度にゲーム性を依存させません。

## 時間管理

ゲームループは OS32 の `get_tick()` と `sys_halt()` を使います。

- 入力: 毎 tick
- 自然落下: gravity timer で独立管理
- 描画: 状態更新後
- busy wait はしない

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
