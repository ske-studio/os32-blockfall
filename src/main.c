#include "os32api.h"
#include "libos32gfx.h"
#include "game.h"
#include "input.h"

/*
 * OS32 Blockfall — main loop.
 * Input is sampled once per tick from kbd_is_pressed(); held keys drive
 * DAS/ARR repeats, edge-triggered keys fire a single action per press.
 * Timing uses get_tick() + sys_halt() (no busy wait).
 * Update order follows AGENTS.md: edge -> held/repeat -> gravity -> lock
 * -> line clear -> spawn -> render.
 */

void main(int argc, char **argv, KernelAPI *api)
{
    Game game;
    InputState input;
    u32 last_tick;

    (void)argc;
    (void)argv;

    libos32gfx_init(api);
    input_init(&input);
    game_init(&game, api->get_tick());
    game_render(&game);

    last_tick = api->get_tick();

    for (;;) {
        while (api->get_tick() == last_tick) {
            api->sys_halt();
        }
        last_tick = api->get_tick();

        input_update(&input, api);

        if (input.escape) break;

        switch (game.mode) {
        case GAME_READY:
            if (input.space) {
                game_start(&game);
            }
            break;

        case GAME_PLAYING: {
            s8 dir;

            /* Edge-triggered actions */
            if (input.rotate_cw) {
                game_rotate_piece(&game, 1);
            }
            if (input.rotate_ccw) {
                game_rotate_piece(&game, 0);
            }
            if (input.space) {
                game_hard_drop(&game);
                break;   /* locked & respawned; rest of update skipped */
            }

            /* Held: horizontal move with game-side auto-repeat */
            dir = input_horizontal_move(&input);
            if (dir != 0) {
                game_move_piece(&game, dir, 0);
            }

            /* Soft drop vs natural gravity */
            if (input.down) {
                game.soft_drop_ticks++;
                if (game.soft_drop_ticks >= 2) {
                    game.soft_drop_ticks = 0;
                    if (!game_move_piece(&game, 0, 1)) {
                        game_lock_piece(&game);
                    }
                }
            } else {
                game.gravity_ticks++;
                if (game.gravity_ticks >= game_get_gravity_ticks(game.level)) {
                    game.gravity_ticks = 0;
                    if (!game_move_piece(&game, 0, 1)) {
                        game_lock_piece(&game);
                    }
                }
            }
            break;
        }

        case GAME_OVER:
            if (input.restart) {
                game_reset(&game);
            }
            break;
        }

        game_render(&game);
    }

    libos32gfx_shutdown();
}