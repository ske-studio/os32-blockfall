#ifndef GAME_H
#define GAME_H

#include "common.h"

/* Logical board (cells) */
#define BOARD_W      10
#define BOARD_H      20
#define PIECE_COUNT  7

/* Screen layout (pixels) */
#define CELL_W      16
#define CELL_H      16

#define PLAY_X      24
#define PLAY_Y      40
#define PLAY_W      240
#define PLAY_H      320

#define BOARD_PIX_X (PLAY_X + 40)
#define BOARD_PIX_Y (PLAY_Y)
#define BOARD_PIX_W (BOARD_W * CELL_W)
#define BOARD_PIX_H (BOARD_H * CELL_H)

/* Right-side status panel */
#define STATUS_X (BOARD_PIX_X + BOARD_PIX_W + 10)
#define STATUS_W 200

typedef enum {
    GAME_READY,
    GAME_PLAYING,
    GAME_OVER
} GameMode;

typedef struct {
    u8 type;   /* 1..7 */
    u8 rot;    /* 0..3 */
    s8 x;      /* board col of 4x4 box origin */
    s8 y;      /* board row of 4x4 box origin */
    u8 color;  /* 1..7 */
} Piece;

typedef struct {
    u8 board[BOARD_H][BOARD_W];
    u8 bag[PIECE_COUNT];   /* 7-bag */
    u8 bag_pos;
    Piece active;
    Piece next_piece;
    u32 score;
    u16 lines;
    u8 level;
    GameMode mode;
    u32 gravity_ticks;
    u32 soft_drop_ticks;
} Game;

/* Initialize game to READY (empty board + first bag piece as NEXT) */
void game_init(Game *game, u32 seed);

/* Reset completely (re-seed, back to READY) */
void game_reset(Game *game);

/* Spawn first piece and enter PLAYING */
int  game_start(Game *game);

/* Spawn the NEXT piece as active, then draw a new NEXT. -1 = game over */
int  game_spawn_piece(Game *game);

/* Valid when piece does not overlap board/bounds */
int  game_is_valid(const Game *game, const Piece *piece);

/* Move piece by dx, dy. 1 = moved, 0 = blocked */
int  game_move_piece(Game *game, s8 dx, s8 dy);

/* Rotate piece (1 = CW, 0 = CCW) with wall kicks. 1 = rotated */
int  game_rotate_piece(Game *game, int clockwise);

/* Drop to the floor, lock and respawn. 1 = done */
int  game_hard_drop(Game *game);

/* Lock active piece into the board, clear lines, spawn next */
void game_lock_piece(Game *game);

/* Clear full rows. Returns number of cleared lines */
int  game_clear_lines(Game *game);

/* Natural gravity interval in ticks for a level */
u32  game_get_gravity_ticks(u8 level);

/* Full redraw + present */
void game_render(const Game *game);

#endif /* GAME_H */