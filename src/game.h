#ifndef GAME_H
#define GAME_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;

#define BOARD_W 10
#define BOARD_H 20
#define PLAY_X 24
#define PLAY_Y 40
#define PLAY_W 240
#define PLAY_H 320
#define BOARD_X (PLAY_X + 40)
#define BOARD_Y PLAY_Y
#define CELL_W 16
#define CELL_H 16

typedef enum {
    GAME_READY,
    GAME_PLAYING,
    GAME_OVER
} GameMode;

typedef struct {
    u8 type;      /* 1..7 */
    u8 rot;       /* 0..3 */
    s8 x;         /* board coord */
    s8 y;         /* board coord */
    u8 color;     /* 1..7 */
} Piece;

typedef struct {
    u8 board[BOARD_H][BOARD_W];
    Piece active;
    Piece next_piece;
    u32 score;
    u16 lines;
    u8 level;
    GameMode mode;
    u32 gravity_ticks;
    u32 soft_drop_ticks;
    u32 lock_delay_ticks;
} Game;

/* Get piece shape based on type and rotation */
static const u8 *get_piece_shape(u8 type, u8 rot);

static const u8 PIECE_COLORS[] = {1, 2, 3, 4, 5, 6, 7};

/* Wall kick offsets for rotation */
static const s8 WALL_KICKS[7] = {-1, 1, -2, 2, 0, 0, 0};

#define PIECE_COUNT 7

/* Initialize game to ready state */
void game_init(Game *game);

/* Reset game completely */
void game_reset(Game *game);

/* Spawn next piece, returns 0 on success, -1 if game over */
int game_spawn_piece(Game *game);

/* Test if piece at position with rotation is valid */
int game_is_valid(const Game *game, const Piece *piece);

/* Move piece by dx, dy, returns 1 if moved, 0 if blocked */
int game_move_piece(Game *game, s8 dx, s8 dy);

/* Rotate piece clockwise */
int game_rotate_piece(Game *game, int clockwise);

/* Hard drop piece */
int game_hard_drop(Game *game);

/* Lock piece to board */
void game_lock_piece(Game *game);

/* Check and clear lines, returns number of lines cleared */
int game_clear_lines(Game *game);

/* Get gravity ticks for current level */
u32 game_get_gravity_ticks(u8 level);

/* Draw entire game state */
void game_render(Game *game);

/* Draw a single block at board coordinates */
void game_draw_block(u16 x, u16 y, u8 color);

#endif /* GAME_H */
