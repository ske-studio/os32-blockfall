#include "game.h"
#include "libos32gfx.h"
#include <stdio.h>

/* Simple pseudo-random number generator (xorshift32) */
static u32 rng_state = 1;

static void rng_seed(u32 seed)
{
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

/* Shuffle array using Fisher-Yates */
static void shuffle(u8 *arr, u8 count)
{
    u8 i;
    for (i = count; i > 1; i--) {
        u32 j = rng_next() % (i + 1);
        u8 tmp = arr[i - 1];
        arr[i - 1] = arr[j];
        arr[j] = tmp;
    }
}

/* Piece shapes (4x4 grid, bitmasks per row) */
static const u8 PIECE_SHAPES[7][4][4] = {
    {{0x0F, 0x0F, 0x0F, 0x0F}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}},
    {{0x3C, 0x3C, 0x00, 0x00}, {0x3C, 0x3C, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}},
    {{0x06, 0x06, 0x06, 0x00}, {0x00, 0x3C, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}},
    {{0x30, 0x3C, 0x00, 0x00}, {0x06, 0x06, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}},
    {{0x06, 0x06, 0x00, 0x00}, {0x30, 0x3C, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}},
    {{0x1E, 0x00, 0x00, 0x00}, {0x3C, 0x3C, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}},
    {{0x00, 0x06, 0x00, 0x00}, {0x00, 0x3C, 0x3C, 0x00}, {0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x00, 0x00}}
};

/* Get piece shape based on type and rotation */
static const u8 *get_piece_shape(u8 type, u8 rot)
{
    const u8 (*shape)[4][4] = PIECE_SHAPES + (type - 1);
    
    /* Rotate the shape matrix */
    if (rot == 0) return (const u8 *)shape[0];
    
    /* Create rotated version on stack */
    static u8 buffer[4][4];
    int r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            if (rot == 1) {
                /* CW: (x, y) -> (-y, x) */
                buffer[r][c] = shape[c][3 - r][0];
            } else if (rot == 2) {
                /* 180: (x, y) -> (-x, -y) */
                buffer[r][c] = shape[3 - r][3 - c][0];
            } else if (rot == 3) {
                /* CCW: (x, y) -> (y, -x) */
                buffer[r][c] = shape[3 - c][r][0];
            } else {
                buffer[r][c] = shape[r][c][0];
            }
        }
    }
    return (const u8 *)buffer;
}

/* Get bounding box of piece at given position and rotation */
static void get_piece_bbox(const u8 (*shape)[4][4], s8 x, s8 y, s8 *minx, s8 *miny, s8 *maxx, s8 *maxy)
{
    *minx = 10;
    *miny = 10;
    *maxx = -10;
    *maxy = -10;
    
    int r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            if ((*shape)[r][c] & 1) {
                s8 px = x + c;
                s8 py = y + r;
                if (px < *minx) *minx = px;
                if (px > *maxx) *maxx = px;
                if (py < *miny) *miny = py;
                if (py > *maxy) *maxy = py;
            }
        }
    }
}

/* Check collision for piece at position */
static int check_collision(const Game *game, const Piece *piece)
{
    const u8 (*shape)[4][4] = PIECE_SHAPES + (piece->type - 1);
    s8 y, x;
    
    /* Check all 4x4 cells of the piece */
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            if ((*shape)[y][x] & 1) {
                s8 board_y = piece->y + y;
                s8 board_x = piece->x + x;
                
                /* Check bounds */
                if (board_x < 0 || board_x >= BOARD_W || board_y >= BOARD_H) {
                    return 1;
                }
                /* Check board collision */
                if (board_y >= 0 && game->board[board_y][board_x] != 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void game_init(Game *game)
{
    rng_seed(12345);
    game->score = 0;
    game->lines = 0;
    game->level = 1;
    game->mode = GAME_READY;
    game->gravity_ticks = 0;
    game->soft_drop_ticks = 0;
    game->lock_delay_ticks = 0;
    
    int y, x;
    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            game->board[y][x] = 0;
        }
    }
}

void game_reset(Game *game)
{
    game_init(game);
}

int game_spawn_piece(Game *game)
{
    u8 types[PIECE_COUNT];
    u8 i;
    for (i = 0; i < PIECE_COUNT; i++) {
        types[i] = i + 1;
    }
    shuffle(types, PIECE_COUNT);
    
    game->next_piece.type = types[0];
    game->next_piece.rot = 0;
    
    /* Spawn at top center */
    game->active.type = types[1];
    game->active.rot = 0;
    game->active.x = 3;
    game->active.y = 0;
    game->active.color = PIECE_COLORS[game->active.type - 1];
    
    /* Check if spawn position is valid */
    if (check_collision(game, &game->active)) {
        game->mode = GAME_OVER;
        return -1;
    }
    
    return 0;
}

int game_is_valid(const Game *game, const Piece *piece)
{
    return !check_collision(game, piece);
}

int game_move_piece(Game *game, s8 dx, s8 dy)
{
    if (game->mode != GAME_PLAYING) return 0;
    
    Piece new_piece = game->active;
    new_piece.x += dx;
    new_piece.y += dy;
    
    if (!check_collision(game, &new_piece)) {
        game->active = new_piece;
        return 1;
    }
    return 0;
}

int game_rotate_piece(Game *game, int clockwise)
{
    if (game->mode != GAME_PLAYING) return 0;
    
    Piece new_piece = game->active;
    new_piece.rot += clockwise ? 1 : -1;
    if (new_piece.rot < 0) new_piece.rot += 4;
    if (new_piece.rot >= 4) new_piece.rot -= 4;
    
    /* Try wall kicks */
    s8 kick;
    
    for (kick = WALL_KICKS[0]; kick <= WALL_KICKS[6]; kick++) {
        new_piece.x += kick;
        if (!check_collision(game, &new_piece)) {
            game->active = new_piece;
            return 1;
        }
        new_piece.x -= kick;
    }
    
    return 0;
}

int game_hard_drop(Game *game)
{
    if (game->mode != GAME_PLAYING) return 0;
    
    while (!check_collision(game, &game->active)) {
        game->active.y++;
    }
    game->active.y--;
    
    /* Lock immediately after hard drop */
    game_lock_piece(game);
    return 1;
}

void game_lock_piece(Game *game)
{
    const u8 (*shape)[4][4] = PIECE_SHAPES + (game->active.type - 1);
    
    int r, c;
    for (r = 0; r < 4; r++) {
        for (c = 0; c < 4; c++) {
            if ((*shape)[r][c] & 1) {
                s8 y = game->active.y + r;
                s8 x = game->active.x + c;
                
                if (y >= 0 && y < BOARD_H && x >= 0 && x < BOARD_W) {
                    game->board[y][x] = game->active.color;
                }
            }
        }
    }
    
    /* Clear active piece */
    game->active.type = 0;
    game->active.rot = 0;
    game->active.x = 0;
    game->active.y = 0;
    game->active.color = 0;
    
    /* Check for game over */
    if (game->board[0][0] != 0) {
        game->mode = GAME_OVER;
        return;
    }
    
    /* Clear lines */
    int cleared = game_clear_lines(game);
    
    if (cleared > 0) {
        /* Update score */
        u32 points = 0;
        if (cleared == 1) points = 100;
        else if (cleared == 2) points = 300;
        else if (cleared == 3) points = 500;
        else if (cleared == 4) points = 800;
        
        game->score += points * game->level;
        game->lines += cleared;
        game->level = game->lines / 10 + 1;
    }
    
    /* Spawn next piece */
    if (game_spawn_piece(game) < 0) {
        game->mode = GAME_OVER;
    }
}

int game_clear_lines(Game *game)
{
    int lines_to_clear[BOARD_H];
    int count = 0;
    
    int y, x, i, j;
    for (y = BOARD_H - 1; y >= 0; y--) {
        int full = 1;
        for (x = 0; x < BOARD_W; x++) {
            if (game->board[y][x] == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            lines_to_clear[count++] = y;
        }
    }
    
    if (count == 0) return 0;
    
    /* Shift rows down */
    int write_pos = count;
    for (i = count - 1; i >= 0; i--) {
        int src = lines_to_clear[i];
        for (j = src + 1; j < write_pos; j++) {
            game->board[j][0] = game->board[j - 1][0];
            for (x = 1; x < BOARD_W; x++) {
                game->board[j][x] = game->board[j - 1][x];
            }
        }
        /* Clear top row */
        for (x = 0; x < BOARD_W; x++) {
            game->board[0][x] = 0;
        }
        write_pos--;
    }
    
    return count;
}

u32 game_get_gravity_ticks(u8 level)
{
    /* Fall speed: 50 ticks/cell at level 1, faster as level increases */
    u32 ticks = 50 - (level - 1) * 5;
    return ticks > 5 ? ticks : 5;
}

void game_draw_block(u16 x, u16 y, u8 color)
{
    /* Draw block with 1px border */
    gfx_rect(x, y, CELL_W - 1, CELL_H - 1, color);
    gfx_fill_rect(x + 1, y + 1, CELL_W - 3, CELL_H - 3, color);
}

void game_render(Game *game)
{
    /* Clear screen */
    gfx_clear(0);
    
    /* Draw play area frame */
    gfx_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 7);
    
    /* Draw board background */
    gfx_fill_rect(BOARD_X, BOARD_Y, BOARD_W, BOARD_H, 15);
    
    /* Draw settled blocks */
    int y, x;
    for (y = 0; y < BOARD_H; y++) {
        for (x = 0; x < BOARD_W; x++) {
            if (game->board[y][x] != 0) {
                game_draw_block(BOARD_X + x * CELL_W, BOARD_Y + y * CELL_H, game->board[y][x]);
            }
        }
    }
    
    /* Draw active piece */
    if (game->active.type != 0 && game->mode == GAME_PLAYING) {
        const u8 (*shape)[4] = get_piece_shape(game->active.type, game->active.rot);
        int r, c;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                if ((shape[r][c] >> 0) & 1) {
                    s8 y = game->active.y + r;
                    s8 x = game->active.x + c;
                    if (y >= 0 && y < BOARD_H && x >= 0 && x < BOARD_W) {
                        game_draw_block(BOARD_X + x * CELL_W, BOARD_Y + y * CELL_H, game->active.color);
                    }
                }
            }
        }
    }
    
    /* Draw status panel */
    gfx_fill_rect(BOARD_X + BOARD_W + 10, PLAY_Y, 200, 320, 14);
    
    kcg_set_scale(1);
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 16, "BLOCKFALL", 15, 0);
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 56, "SCORE", 7, 0);
    
    char score_buf[16];
    snprintf(score_buf, sizeof(score_buf), "%u", game->score);
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 80, score_buf, 15, 0);
    
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 120, "LINES", 7, 0);
    char lines_buf[16];
    snprintf(lines_buf, sizeof(lines_buf), "%u", game->lines);
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 144, lines_buf, 15, 0);
    
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 184, "LEVEL", 7, 0);
    char level_buf[16];
    snprintf(level_buf, sizeof(level_buf), "%u", game->level);
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 208, level_buf, 15, 0);
    
    kcg_draw_utf8(BOARD_X + BOARD_W + 20, PLAY_Y + 248, "NEXT", 7, 0);
    
    /* Draw next piece preview */
    if (game->next_piece.type != 0) {
        gfx_fill_rect(BOARD_X + BOARD_W + 20, PLAY_Y + 272, 160, 64, 13);
        const u8 (*shape)[4] = get_piece_shape(game->next_piece.type, 0);
        int r, c;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++) {
                if ((shape[r][c] >> 0) & 1) {
                    gfx_fill_rect(BOARD_X + BOARD_W + 48 + c * 12, PLAY_Y + 288 + r * 12, 10, 10, game->next_piece.color);
                }
            }
        }
    }
    
    /* Draw mode overlay */
    if (game->mode == GAME_READY) {
        gfx_fill_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 1);
        kcg_set_scale(1);
        kcg_draw_utf8(BOARD_X + 40, BOARD_Y + 120, "PRESS SPACE", 14, 0);
    } else if (game->mode == GAME_OVER) {
        gfx_fill_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 1);
        kcg_set_scale(1);
        kcg_draw_utf8(BOARD_X + 40, BOARD_Y + 80, "GAME OVER", 15, 0);
        kcg_draw_utf8(BOARD_X + 40, BOARD_Y + 120, "Press R to restart", 14, 0);
    }
    
    gfx_present();
}
