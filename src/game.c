#include "game.h"
#include "libos32gfx.h"
#include <stdio.h>

/* ======================================================================== */
/*  RNG (xorshift32)                                                        */
/* ======================================================================== */

static u32 rng_state;

static void rng_seed(u32 seed)
{
    /* 0 -> 1 に補正しないと xorshift が 0 に固まる */
    if (seed == 0) seed = 1;
    rng_state = seed;
}

static u32 rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

/* ======================================================================== */
/*  Piece definitions (4x4 local grid, x right / y down)                    */
/* ======================================================================== */

typedef struct {
    s8 x;
    s8 y;
} Cell;

/* PIECE_CELLS[type-1][rot][cell] — precomputed for all 4 rotations. */
static const Cell PIECE_CELLS[PIECE_COUNT][4][4] = {
    /* 1: I */
    {
        {{0,1},{1,1},{2,1},{3,1}},
        {{2,0},{2,1},{2,2},{2,3}},
        {{0,2},{1,2},{2,2},{3,2}},
        {{1,0},{1,1},{1,2},{1,3}},
    },
    /* 2: O */
    {
        {{1,1},{2,1},{1,2},{2,2}},
        {{1,1},{2,1},{1,2},{2,2}},
        {{1,1},{2,1},{1,2},{2,2}},
        {{1,1},{2,1},{1,2},{2,2}},
    },
    /* 3: T */
    {
        {{1,0},{0,1},{1,1},{2,1}},
        {{2,0},{2,1},{2,2},{3,1}},
        {{2,3},{1,2},{2,2},{3,2}},
        {{1,1},{1,2},{1,3},{0,2}},
    },
    /* 4: S */
    {
        {{1,0},{2,0},{0,1},{1,1}},
        {{2,0},{2,1},{3,1},{3,2}},
        {{3,2},{2,2},{1,3},{2,3}},
        {{0,1},{0,2},{1,2},{1,3}},
    },
    /* 5: Z */
    {
        {{0,0},{1,0},{1,1},{2,1}},
        {{3,0},{3,1},{2,1},{2,2}},
        {{3,3},{2,3},{2,2},{1,2}},
        {{0,3},{0,2},{1,2},{1,1}},
    },
    /* 6: J */
    {
        {{0,0},{0,1},{1,1},{2,1}},
        {{3,0},{2,0},{2,1},{2,2}},
        {{3,3},{3,2},{2,2},{1,2}},
        {{0,3},{1,3},{1,2},{1,1}},
    },
    /* 7: L */
    {
        {{2,0},{0,1},{1,1},{2,1}},
        {{3,2},{2,0},{2,1},{2,2}},
        {{1,3},{3,2},{2,2},{1,2}},
        {{0,1},{1,3},{1,2},{1,1}},
    },
};

static const u8 PIECE_COLORS[PIECE_COUNT] = {1, 2, 3, 4, 5, 6, 7};

/* Wall kick offsets, in the order from SPEC§5 */
static const s8 KICKS[6][2] = {
    {0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1},
};

/* ======================================================================== */
/*  Collision / 7-bag helpers                                               */
/* ======================================================================== */

static int piece_collides(const Game *game, const Piece *piece)
{
    const Cell *cells = PIECE_CELLS[piece->type - 1][piece->rot & 3];
    int i;

    for (i = 0; i < 4; i++) {
        int bx = piece->x + cells[i].x;
        int by = piece->y + cells[i].y;

        if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return 1;
        if (by >= 0 && game->board[by][bx] != 0) return 1;
    }
    return 0;
}

static void bag_fill(Game *game)
{
    int i;

    for (i = 0; i < PIECE_COUNT; i++) game->bag[i] = (u8)(i + 1);
    for (i = PIECE_COUNT - 1; i > 0; i--) {
        u32 j = rng_next() % (u32)(i + 1);
        u8 t = game->bag[i];
        game->bag[i] = game->bag[j];
        game->bag[j] = t;
    }
    game->bag_pos = 0;
}

static u8 bag_take(Game *game)
{
    if (game->bag_pos >= PIECE_COUNT) bag_fill(game);
    return game->bag[game->bag_pos++];
}

/* ======================================================================== */
/*  Public API                                                              */
/* ======================================================================== */

void game_init(Game *game, u32 seed)
{
    int y, x;

    rng_seed(seed);
    game->score = 0;
    game->lines = 0;
    game->level = 1;
    game->mode = GAME_READY;
    game->gravity_ticks = 0;
    game->soft_drop_ticks = 0;

    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            game->board[y][x] = 0;
        }
    }

    bag_fill(game);
    game->next_piece.type = bag_take(game);
    game->next_piece.rot = 0;
    game->next_piece.color = PIECE_COLORS[game->next_piece.type - 1];

    game->active.type = 0;
    game->active.rot = 0;
    game->active.x = 0;
    game->active.y = 0;
    game->active.color = 0;
}

void game_reset(Game *game)
{
    game_init(game, rng_next());
}

int game_start(Game *game)
{
    if (game->mode != GAME_READY) return -1;
    game->mode = GAME_PLAYING;
    return game_spawn_piece(game);
}

int game_spawn_piece(Game *game)
{
    game->active = game->next_piece;
    game->active.x = 3;
    game->active.y = 0;
    game->gravity_ticks = 0;
    game->soft_drop_ticks = 0;

    /* Prepare the new NEXT */
    game->next_piece.type = bag_take(game);
    game->next_piece.rot = 0;
    game->next_piece.color = PIECE_COLORS[game->next_piece.type - 1];

    if (piece_collides(game, &game->active)) {
        game->mode = GAME_OVER;
        return -1;
    }
    return 0;
}

int game_is_valid(const Game *game, const Piece *piece)
{
    return !piece_collides(game, piece);
}

int game_move_piece(Game *game, s8 dx, s8 dy)
{
    Piece np;

    if (game->mode != GAME_PLAYING) return 0;

    np = game->active;
    np.x += dx;
    np.y += dy;

    if (piece_collides(game, &np)) return 0;

    game->active = np;
    return 1;
}

int game_rotate_piece(Game *game, int clockwise)
{
    Piece np;
    int k;

    if (game->mode != GAME_PLAYING) return 0;
    if (game->active.type == 2) return 1;   /* O piece: no visible rotation */

    np = game->active;
    if (clockwise) np.rot = (u8)((np.rot + 1) & 3);
    else           np.rot = (u8)((np.rot + 3) & 3);

    for (k = 0; k < 6; k++) {
        Piece trial = np;
        trial.x += KICKS[k][0];
        trial.y += KICKS[k][1];
        if (!piece_collides(game, &trial)) {
            game->active = trial;
            return 1;
        }
    }
    return 0;
}

int game_hard_drop(Game *game)
{
    if (game->mode != GAME_PLAYING) return 0;

    while (game_move_piece(game, 0, 1)) {
    }

    game_lock_piece(game);
    return 1;
}

void game_lock_piece(Game *game)
{
    const Cell *cells = PIECE_CELLS[game->active.type - 1][game->active.rot & 3];
    int i;

    for (i = 0; i < 4; i++) {
        int bx = game->active.x + cells[i].x;
        int by = game->active.y + cells[i].y;

        if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
            game->board[by][bx] = game->active.color;
        }
    }

    game->active.type = 0;
    game->active.rot = 0;
    game->active.x = 0;
    game->active.y = 0;
    game->active.color = 0;

    {
        int cleared = game_clear_lines(game);

        if (cleared > 0) {
            static const u32 BASE_SCORE[5] = {0, 100, 300, 500, 800};
            game->score += BASE_SCORE[cleared] * game->level;
            game->lines += (u16)cleared;
            game->level = (u8)(game->lines / 10 + 1);
        }
    }

    if (game_spawn_piece(game) < 0) {
        game->mode = GAME_OVER;
    }
}

int game_clear_lines(Game *game)
{
    int src, dst, x;
    int cleared = 0;

    dst = BOARD_H - 1;
    for (src = BOARD_H - 1; src >= 0; src--) {
        int full = 1;

        for (x = 0; x < BOARD_W; x++) {
            if (game->board[src][x] == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            cleared++;
            continue;
        }
        if (dst != src) {
            for (x = 0; x < BOARD_W; x++) {
                game->board[dst][x] = game->board[src][x];
            }
        }
        dst--;
    }
    for (; dst >= 0; dst--) {
        for (x = 0; x < BOARD_W; x++) {
            game->board[dst][x] = 0;
        }
    }
    return cleared;
}

u32 game_get_gravity_ticks(u8 level)
{
    u32 ticks = 50 - (u32)(level - 1) * 5;
    return ticks > 5 ? ticks : 5;
}

/* ======================================================================== */
/*  Rendering                                                               */
/* ======================================================================== */

static void draw_block(int px, int py, u8 color)
{
    gfx_rect(px, py, CELL_W - 1, CELL_H - 1, color);
    gfx_fill_rect(px + 1, py + 1, CELL_W - 3, CELL_H - 3, color);
}

static void draw_board(const Game *game)
{
    int y, x;

    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            if (game->board[y][x] != 0) {
                draw_block(BOARD_PIX_X + x * CELL_W,
                           BOARD_PIX_Y + y * CELL_H, game->board[y][x]);
            }
        }
    }
}

static void draw_active(const Game *game)
{
    const Cell *cells;
    int i;

    if (game->active.type == 0 || game->mode != GAME_PLAYING) return;

    cells = PIECE_CELLS[game->active.type - 1][game->active.rot & 3];
    for (i = 0; i < 4; i++) {
        int bx = game->active.x + cells[i].x;
        int by = game->active.y + cells[i].y;

        if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H) {
            draw_block(BOARD_PIX_X + bx * CELL_W,
                       BOARD_PIX_Y + by * CELL_H, game->active.color);
        }
    }
}

static void draw_text_num(int x, int y, const char *label, u32 value)
{
    char buf[16];

    kcg_set_scale(1);
    kcg_draw_utf8(x, y, label, 7, 0);
    snprintf(buf, sizeof(buf), "%u", (unsigned int)value);
    kcg_draw_utf8(x, y + 24, buf, 15, 0);
}

static void draw_status(const Game *game)
{
    kcg_set_scale(1);
    gfx_fill_rect(STATUS_X, PLAY_Y, STATUS_W, PLAY_H, 14);
    kcg_draw_utf8(STATUS_X + 10, PLAY_Y + 16, "BLOCKFALL", 15, 0);
    draw_text_num(STATUS_X + 10, PLAY_Y + 56, "SCORE", game->score);
    draw_text_num(STATUS_X + 10, PLAY_Y + 120, "LINES", game->lines);
    draw_text_num(STATUS_X + 10, PLAY_Y + 184, "LEVEL", game->level);
    kcg_set_scale(1);
    kcg_draw_utf8(STATUS_X + 10, PLAY_Y + 248, "NEXT", 7, 0);
}

static void draw_next(const Game *game)
{
    const Cell *cells;
    int i;

    if (game->next_piece.type == 0) return;

    gfx_fill_rect(STATUS_X + 10, PLAY_Y + 272, 160, 64, 13);
    cells = PIECE_CELLS[game->next_piece.type - 1][0];
    for (i = 0; i < 4; i++) {
        gfx_fill_rect(STATUS_X + 38 + cells[i].x * 12,
                      PLAY_Y + 288 + cells[i].y * 12,
                      10, 10, game->next_piece.color);
    }
}

static void draw_overlay(const Game *game)
{
    kcg_set_scale(1);

    if (game->mode == GAME_READY) {
        gfx_fill_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 1);
        kcg_draw_utf8(BOARD_PIX_X + 40, BOARD_PIX_Y + 120, "PRESS SPACE", 14, 0);
    } else if (game->mode == GAME_OVER) {
        gfx_fill_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 1);
        kcg_draw_utf8(BOARD_PIX_X + 40, BOARD_PIX_Y + 80, "GAME OVER", 15, 0);
        kcg_draw_utf8(BOARD_PIX_X + 40, BOARD_PIX_Y + 120, "Press R to restart", 14, 0);
    }
}

void game_render(const Game *game)
{
    gfx_clear(0);

    /* Play area frame + board background */
    gfx_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 7);
    gfx_fill_rect(BOARD_PIX_X, BOARD_PIX_Y, BOARD_PIX_W, BOARD_PIX_H, 15);

    draw_board(game);
    draw_active(game);
    draw_status(game);
    draw_next(game);
    draw_overlay(game);

    gfx_present();
}