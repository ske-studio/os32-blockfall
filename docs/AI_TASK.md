# AI Implementation Task

## Goal

Implement the Blockfall MVP described in `docs/SPEC.md` as an OS32 userland application.

The task is intentionally constrained. Complete the specified game before adding optional features.

## Required outcome

The repository must build a runnable OS32 binary:

```text
build/blockfall.bin
```

The application must support a complete play cycle:

```text
READY -> PLAYING -> GAME_OVER -> restart
```

## Required implementation order

1. Read `README.md`, `docs/SPEC.md`, `docs/AI_TASK.md`, `docs/TEST_PLAN.md`.
2. Confirm every OS32 SDK API used by the scaffold actually exists.
3. Make the project build with the standalone Makefile.
4. Draw static 240×320 play area and right-side status panel.
5. Implement 10×20 board rendering.
6. Implement piece definitions and spawn.
7. Implement collision test.
8. Implement held-key input with `kbd_is_pressed()`.
9. Implement edge detection for rotation / hard drop.
10. Implement game-side left/right repeat (DAS/ARR).
11. Implement gravity and soft drop independent from the render loop.
12. Implement locking and spawning the next piece.
13. Implement line clear for 1–4 simultaneous rows.
14. Implement score / lines / level.
15. Implement NEXT preview.
16. Implement GAME OVER and restart.
17. Verify all items in `docs/TEST_PLAN.md`.
18. Update documentation only to match verified implementation facts.

## Breakout lessons that must be preserved

### Build

Do not hard-code a GCC version directory such as:

```text
.../lib/gcc/i386-elf/13.2.0
```

Use the Breakout solution: derive the toolchain root from `i386-elf-gcc` on PATH and ask the compiler for the real libgcc path.

### Input

Do not implement gameplay movement using only `kbd_trygetchar()` and OS keyboard repeat.

- held state: `kbd_is_pressed()`
- edge-triggered actions: compare current vs previous key state
- left/right repeat: game timer

### Timing

Do not tie falling speed to CPU speed, present count, or a busy loop.

Use:

```text
get_tick()
sys_halt()
```

Input sampling and gravity timing must be separate.

### Shutdown

All normal exit paths must reach `libos32gfx_shutdown()`.

## Suggested source structure

A compact split is preferred:

```text
src/
├── main.c       game loop / OS32 integration
├── game.c       board, pieces, collision, score
├── game.h
├── input.c      key state / edge / repeat
└── input.h
```

A single `main.c` is acceptable for an initial milestone, but do not create a general engine layer.

## Suggested state

```c
typedef enum {
    GAME_READY,
    GAME_PLAYING,
    GAME_OVER
} GameMode;

typedef struct {
    int type;
    int rot;
    int x;
    int y;
} ActivePiece;

typedef struct {
    u8 board[20][10];
    ActivePiece active;
    int next_type;
    int score;
    int lines;
    int level;
    GameMode mode;
    u32 gravity_ticks;
} Game;
```

These names are suggestions, not an ABI.

## Rendering guidance

The MVP may redraw the full frame each tick. Optimize only after correctness.

Keep logical board coordinates independent of pixels.

```text
board cell -> screen coordinate
```

must be done in drawing code, not embedded throughout game logic.

## Definition of done

Implementation is done only when:

- `make` succeeds;
- `build/blockfall.bin` is generated;
- it launches under OS32;
- READY works;
- all seven pieces can spawn and move;
- clockwise and counter-clockwise rotation work;
- left/right held movement is smooth and independent of OS keyboard repeat;
- soft drop and hard drop work;
- pieces lock correctly;
- 1, 2, 3, and 4 simultaneous lines can clear correctly;
- score, lines, level, and NEXT update correctly;
- spawn collision causes GAME OVER;
- `R` restarts from GAME OVER;
- `Esc` exits cleanly;
- no optional feature was added at the expense of MVP correctness.

## Completion report

At completion, report:

1. changed files;
2. implemented features;
3. build/test commands actually run;
4. emulator/real-machine items not verified;
5. known limitations.
