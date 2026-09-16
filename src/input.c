#include "input.h"

static u8 key_held(KernelAPI *api, int sc)
{
    if (!api || !api->kbd_is_pressed) return 0;
    return (u8)api->kbd_is_pressed(sc);
}

void input_init(InputState *st)
{
    st->left = 0;
    st->right = 0;
    st->down = 0;

    st->rotate_cw = 0;
    st->rotate_ccw = 0;
    st->space = 0;
    st->restart = 0;
    st->escape = 0;

    st->p_rotate_cw = 0;
    st->p_rotate_ccw = 0;
    st->p_space = 0;
    st->p_restart = 0;
    st->p_escape = 0;

    st->auto_dir = 0;
    st->das_timer = 0;
    st->arr_timer = 0;
    st->das = DAS_TICKS;
    st->arr = ARR_TICKS;
}

void input_update(InputState *st, KernelAPI *api)
{
    u8 rotate_cw  = key_held(api, KEY_UP) || key_held(api, KEY_X);
    u8 rotate_ccw = key_held(api, KEY_Z);
    u8 space      = key_held(api, KEY_SPACE);
    u8 restart    = key_held(api, KEY_R);
    u8 escape     = key_held(api, KEY_ESC);

    st->left  = key_held(api, KEY_LEFT) || key_held(api, KEY_A);
    st->right = key_held(api, KEY_RIGHT) || key_held(api, KEY_D);
    st->down  = key_held(api, KEY_DOWN) || key_held(api, KEY_S);

    /* edge = current && !previous */
    st->rotate_cw  = rotate_cw  && !st->p_rotate_cw;
    st->rotate_ccw = rotate_ccw && !st->p_rotate_ccw;
    st->space      = space      && !st->p_space;
    st->restart    = restart    && !st->p_restart;
    st->escape     = escape     && !st->p_escape;

    st->p_rotate_cw  = rotate_cw;
    st->p_rotate_ccw = rotate_ccw;
    st->p_space      = space;
    st->p_restart    = restart;
    st->p_escape     = escape;
}

s8 input_horizontal_move(InputState *st)
{
    s8 dir;

    if (st->left && !st->right)       dir = -1;
    else if (st->right && !st->left)  dir = 1;
    else                              dir = 0;

    if (dir == 0) {
        st->auto_dir = 0;
        st->das_timer = 0;
        st->arr_timer = 0;
        return 0;
    }

    /* New press or direction change: move immediately, start DAS */
    if (st->auto_dir != dir) {
        st->auto_dir = dir;
        st->das_timer = 0;
        st->arr_timer = 0;
        return dir;
    }

    /* Held: wait out DAS, then repeat every ARR ticks */
    if (st->das_timer < st->das) {
        st->das_timer++;
        return 0;
    }
    st->arr_timer++;
    if (st->arr_timer < st->arr) return 0;
    st->arr_timer = 0;
    return dir;
}