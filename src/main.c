#include "os32api.h"
#include "libos32gfx.h"
#include "game.h"
#include "input.h"
#include <stdlib.h>

#define SCREEN_W 640
#define SCREEN_H 400

#define PLAY_X 24
#define PLAY_Y 40
#define PLAY_W 240
#define PLAY_H 320

#define BOARD_X (PLAY_X + 40)
#define BOARD_Y PLAY_Y
#define CELL_W 16
#define CELL_H 16

static Game *game = NULL;
static InputState input;
static u32 last_tick = 0;
static u32 tick_counter = 0;

void game_loop(void)
{
    static KernelAPI api;
    
    /* Initialize */
    game = malloc(sizeof(Game));
    if (!game) {
        goto done;
    }
    game_init(game);
    input_init(&input);
    
    libos32gfx_init(&api);
    game_render(game);
    
    last_tick = api.get_tick();
    
    for (;;) {
        u32 current_tick = api.get_tick();
        
        /* Process input */
        int ch;
        while ((ch = api.kbd_trygetchar()) >= 0) {
            if (ch == SC_ESCAPE) {
                goto done;
            }
            input_poll(&input, ch);
        }
        
        /* Wait for next tick */
        while (current_tick == last_tick) {
            api.sys_halt();
        }
        last_tick = current_tick;
        tick_counter++;
        
        /* Update input state timers */
        if (input.left_pressed) {
            input.left_repeat_timer = 0;
        } else {
            input.left_repeat_timer++;
        }
        
        if (input.right_pressed) {
            input.right_repeat_timer = 0;
        } else {
            input.right_repeat_timer++;
        }
        
        /* Handle held movement with DAS/ARR */
        if (input.left_pressed && input.left_repeat_timer >= input.repeat_delay) {
            game_move_piece(game, -1, 0);
            input.left_repeat_timer = 0;
        }
        
        if (input.right_pressed && input.right_repeat_timer >= input.repeat_delay) {
            game_move_piece(game, 1, 0);
            input.right_repeat_timer = 0;
        }
        
        /* Handle soft drop */
        if (input.down_pressed) {
            game_move_piece(game, 0, 1);
            game->soft_drop_ticks = 0;
        } else {
            game->soft_drop_ticks++;
        }
        
        /* Handle rotation (edge-triggered) */
        u8 up_was_pressed = input.up_pressed;
        if (input.up_pressed && !up_was_pressed) {
            game_rotate_piece(game, 1);
        }
        
        u8 z_was_pressed = input.z_pressed;
        if (input.z_pressed && !z_was_pressed) {
            game_rotate_piece(game, 0);
        }
        
        /* Handle hard drop (edge-triggered) */
        u8 space_was_pressed = input.space_pressed;
        if (input.space_pressed && !space_was_pressed) {
            game_hard_drop(game);
        }
        
        /* Handle restart (edge-triggered) */
        u8 r_was_pressed = input.r_pressed;
        if (input.r_pressed && !r_was_pressed) {
            if (game->mode == GAME_OVER) {
                game_reset(game);
                game_render(game);
            }
        }
        
        /* Handle escape (edge-triggered) */
        u8 esc_was_pressed = input.escape_pressed;
        if (input.escape_pressed && !esc_was_pressed) {
            goto done;
        }
        
        /* Update gravity */
        u32 gravity_ticks = game_get_gravity_ticks(game->level);
        
        if (game->mode == GAME_PLAYING) {
            if (input.down_pressed) {
                /* Soft drop: faster fall */
                if (game->soft_drop_ticks >= 2) {
                    game_move_piece(game, 0, 1);
                    game->soft_drop_ticks = 0;
                }
            } else {
                /* Natural gravity */
                game->gravity_ticks++;
                if (game->gravity_ticks >= gravity_ticks) {
                    game->gravity_ticks = 0;
                    game_move_piece(game, 0, 1);
                }
            }
        }
        
        /* Render */
        game_render(game);
    }
    
done:
    libos32gfx_shutdown();
}

void main(int argc, char **argv, KernelAPI *api)
{
    (void)argc;
    (void)argv;
    
    game_loop();
}
