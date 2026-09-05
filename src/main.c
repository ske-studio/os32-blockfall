#include "os32api.h"
#include "libos32gfx.h"

/*
 * OS32 Blockfall — initial scaffold.
 * Gameplay implementation must follow docs/SPEC.md.
 */

#define SCREEN_W 640
#define SCREEN_H 400

#define PLAY_X 24
#define PLAY_Y 40
#define PLAY_W 240
#define PLAY_H 320

#define BOARD_X (PLAY_X + 40)
#define BOARD_Y PLAY_Y
#define BOARD_W 160
#define BOARD_H 320

static void draw_scaffold(void)
{
    gfx_clear(0);

    gfx_rect(PLAY_X, PLAY_Y, PLAY_W, PLAY_H, 7);
    gfx_rect(BOARD_X, BOARD_Y, BOARD_W, BOARD_H, 15);

    kcg_set_scale(1);
    kcg_draw_utf8(304, 48, "BLOCKFALL", 15, 0);
    kcg_draw_utf8(304, 88, "SCORE", 7, 0);
    kcg_draw_utf8(304, 112, "000000", 15, 0);
    kcg_draw_utf8(304, 152, "LINES", 7, 0);
    kcg_draw_utf8(304, 176, "000", 15, 0);
    kcg_draw_utf8(304, 216, "LEVEL", 7, 0);
    kcg_draw_utf8(304, 240, "01", 15, 0);
    kcg_draw_utf8(304, 280, "NEXT", 7, 0);

    kcg_draw_utf8(BOARD_X + 32, BOARD_Y + 144, "PRESS SPACE", 14, 0);

    gfx_present();
}

void main(int argc, char **argv, KernelAPI *api)
{
    int ch;
    u32 last_tick;

    (void)argc;
    (void)argv;

    libos32gfx_init(api);
    draw_scaffold();

    last_tick = api->get_tick();

    for (;;) {
        while ((ch = api->kbd_trygetchar()) >= 0) {
            if (ch == 0x1B) {
                goto done;
            }
        }

        while (api->get_tick() == last_tick) {
            api->sys_halt();
        }
        last_tick = api->get_tick();
    }

done:
    libos32gfx_shutdown();
}
