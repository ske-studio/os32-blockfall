#ifndef INPUT_H
#define INPUT_H

#include "common.h"
#include "os32api.h"

/* PC-98 key codes (mirror of os32 drivers/kbd.h) */
#define KEY_ESC    0x00
#define KEY_R      0x13
#define KEY_A      0x1D
#define KEY_S      0x1E
#define KEY_D      0x1F
#define KEY_Z      0x29
#define KEY_X      0x2A
#define KEY_SPACE  0x34
#define KEY_UP     0x3A
#define KEY_LEFT   0x3B
#define KEY_RIGHT  0x3C
#define KEY_DOWN   0x3D

/* Horizontal auto-repeat (SPEC§9) */
#define DAS_TICKS  15
#define ARR_TICKS  4

typedef struct {
    /* held state (this tick) */
    u8 left;       /* Left / A */
    u8 right;      /* Right / D */
    u8 down;       /* Down / S  (soft drop) */

    /* edge state (this tick: current && !prev) */
    u8 rotate_cw;   /* Up / X */
    u8 rotate_ccw;  /* Z */
    u8 space;       /* Space: start (READY) / hard drop (PLAYING) */
    u8 restart;     /* R */
    u8 escape;      /* Esc */

    /* previous held, for edge detection */
    u8 p_rotate_cw;
    u8 p_rotate_ccw;
    u8 p_space;
    u8 p_restart;
    u8 p_escape;

    /* DAS / ARR */
    s8 auto_dir;
    u8 das_timer;
    u8 arr_timer;
    u8 das;
    u8 arr;
} InputState;

void input_init(InputState *state);

/* Sample held keys via kbd_is_pressed() and compute edges. Call once per tick. */
void input_update(InputState *state, KernelAPI *api);

/* Horizontal auto-repeat step for this tick: -1 / 0 / +1 */
s8 input_horizontal_move(InputState *state);

#endif /* INPUT_H */