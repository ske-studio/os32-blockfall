# Blockfall MVP Specification

## 1. Goal

OS32 上で動作する、シンプルな落ち物パズルゲームを実装する。

ゲームの目的は、落下する 4 ブロックピースを盤面へ配置し、横 1 行を完全に埋めて消去し、ゲームオーバーまでスコアを伸ばすこと。

本仕様は MVP を定義する。仕様外の機能を実装する場合は別途承認する。

---

## 2. Platform / implementation constraints

- OS32 userland application
- C / GNU89 / freestanding
- OS32 SDK の公開 API のみ使用
- 描画は `libos32gfx`
- 入力は `KernelAPI`
- 浮動小数点禁止
- 外部依存・外部画像・外部音声禁止
- OS32 本体を変更しない

---

## 3. Screen layout

OS32 画面: 640×400 px。

### Play area

- 左側
- 幅 240 px
- 高さ 320 px
- 盤面は 10 列 × 20 行
- 1 cell = 16×16 px
- 盤面実サイズ = 160×320 px
- play area の左右に 40 px ずつ余白を取る

推奨配置:

```text
PLAY_X = 24
PLAY_Y = 40
PLAY_W = 240
PLAY_H = 320
BOARD_X = PLAY_X + 40
BOARD_Y = PLAY_Y
BOARD_W = 160
BOARD_H = 320
```

### Status area

右側に以下を表示する。

- `BLOCKFALL`
- SCORE
- LINES
- LEVEL
- NEXT
- 操作説明（余裕があれば）

---

## 4. Board model

論理盤面:

```c
u8 board[20][10];
```

- 0 = empty
- 1..7 = piece/color id

座標:

```text
x: 0..9
y: 0..19
```

画面座標:

```text
px = BOARD_X + x * 16
py = BOARD_Y + y * 16
```

---

## 5. Pieces

MVP では 7 種類の tetromino 相当を使う。

- I
- O
- T
- S
- Z
- J
- L

各ピースは 4×4 のローカルグリッドまたは 4 cell の座標集合で定義する。

回転状態は最大 4。

MVP では Super Rotation System の完全互換は不要。単純回転 + 最小限の wall kick でよい。

### Required wall kick

回転後に衝突する場合、以下の順に試してよい。

```text
(0, 0)
(-1, 0)
(+1, 0)
(-2, 0)
(+2, 0)
(0, -1)
```

どれも置けなければ回転しない。

O piece は見た目上回転不要。

---

## 6. Piece generation

MVP では 7-bag を推奨する。

- 7 種を 1 個ずつ配列へ入れる
- shuffle
- 順番に取り出す
- 空になったら再生成

乱数は OS32 SDK で利用可能な手段だけを使うこと。未確認 API を作らない。

利用可能な乱数 API が無い場合は `get_tick()` を seed にした簡単な xorshift32 をアプリ側に実装してよい。

NEXT は少なくとも次の 1 piece を表示する。

---

## 7. Game states

```text
READY
PLAYING
GAME_OVER
```

### READY

- 空盤面
- 最初のピースを準備
- `PRESS SPACE` を表示
- Space で PLAYING

### PLAYING

通常ゲーム。

### GAME_OVER

新しいピースを spawn できないと GAME_OVER。

- 最終 SCORE を表示
- `R` で完全初期化して READY
- `Esc` で終了

---

## 8. Spawn

新しいピースは盤面上部中央付近に出す。

推奨:

```text
piece.x = 3
piece.y = 0
```

piece の形によって初期位置を微調整してよい。

spawn 時点で盤面または境界と衝突する場合は GAME_OVER。

---

## 9. Input

OS32 Breakout で文字入力の OS リピート依存を避ける必要があったため、本ゲームは key state と edge detection を基本とする。

### Required controls

```text
Left / A     move left
Right / D    move right
Down / S     soft drop
Up / X       rotate clockwise
Z            rotate counter-clockwise (任意ではなく MVP に含める)
Space        hard drop / READY start
R            restart on GAME_OVER
Esc          exit
```

### Held input

Left / Right / Soft drop は `kbd_is_pressed(scancode)` を使用する。

左右移動はゲーム側で key repeat を制御する。

推奨値:

```text
initial move: immediate
DAS: 15 ticks
ARR: 4 ticks
```

100Hz tick を前提とすると、およそ 150ms 後に 40ms 間隔。

値はプレイ感に応じて微調整可。

### Edge input

回転 / hard drop / restart は、前 tick の key state と比較して押下 edge だけで 1 回実行する。

```c
pressed = current && !previous;
```

押しっぱなしで毎 tick 回転・hard drop してはならない。

---

## 10. Timing / gravity

ゲームループ自体は OS32 tick 単位で回す。

```text
sample input every tick
update timers every tick
gravity only when gravity counter expires
wait with sys_halt()
```

busy wait 禁止。

### Gravity

LEVEL 1 の推奨自然落下:

```text
50 ticks / cell = 0.5 sec / cell
```

LEVEL が上がるごとに速くする。

MVP の例:

```text
fall_ticks = max(5, 50 - (level - 1) * 5)
```

正確な既存ゲーム互換カーブは不要。

### Soft drop

Down/S を押している間は自然落下より速くする。

推奨:

```text
2 ticks / cell
```

### Hard drop

Space 押下で、衝突直前まで即座に落とし、その tick で固定する。

---

## 11. Collision

ピースの各 occupied cell について以下を確認する。

- x < 0 → invalid
- x >= 10 → invalid
- y >= 20 → invalid
- y >= 0 かつ `board[y][x] != 0` → invalid

上端より上の cell (`y < 0`) は spawn/rotation 中のみ許容してよい。

---

## 12. Lock

下方向へ 1 cell 動かせない場合、ピースを盤面へ固定する。

MVP では複雑な lock delay は不要。

ただし操作感改善のため、短い lock delay を入れてもよい。

推奨上限:

```text
LOCK_DELAY = 30 ticks
```

実装する場合も、床上で無限に回転・移動できる仕組みは不要。

最初の MVP は「gravity/soft drop で下へ進めなくなったら固定」でも合格とする。

---

## 13. Line clear

固定後に全 20 行を調べる。

1 行の 10 cell がすべて非 0 なら complete。

complete line を削除し、それより上の行を 1 行ずつ下へ詰める。

同時に複数行消えることを正しく処理する。

---

## 14. Score

MVP スコア:

```text
1 line:   100 × level
2 lines:  300 × level
3 lines:  500 × level
4 lines:  800 × level
```

Soft drop / hard drop の距離加点は MVP では不要。

T-spin / combo / back-to-back は不要。

---

## 15. Lines / Level

`lines` は累積消去ライン数。

```text
level = lines / 10 + 1
```

10 ラインごとに level up。

level に応じて gravity を速くする。

---

## 16. Rendering

MVP では全画面 redraw + `gfx_present()` でもよい。

描画順:

```text
clear background
play-area frame
settled board
active piece
status panel
NEXT preview
READY / GAME OVER overlay
present
```

1 block は 16×16 px。

見やすさのため、各 block は例えば 14×14 の塗り + 1px 枠でもよい。

7 piece は OS32 の既存 16 色パレットから別色を割り当てる。

---

## 17. Performance

- 毎 tick の全盤面走査は 20×10 = 200 cell なので許容。
- 動的メモリ不要。
- ピース衝突判定は最大 4 cell。
- busy wait 禁止。
- ゲーム速度を描画回数や CPU 実行速度へ依存させない。

---

## 18. Shutdown

正常終了時には必ず:

```c
libos32gfx_shutdown();
```

を呼ぶ。

---

## 19. Explicit non-goals

MVP では以下を実装しない。

- HOLD
- ghost piece
- T-spin 判定
- combo / back-to-back
- guideline 完全互換 SRS
- title/menu system
- BGM / sound effects
- high score persistence
- network
- mouse
- external assets
- particle effects

これらは MVP 完了後の追加機能とする。
