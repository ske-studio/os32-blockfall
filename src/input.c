#include "input.h"
#include "os32api.h"

#define DAS 15   /* Delay before repeat starts (ticks) */
#define ARR 4    /* Auto-repeat rate (ticks) */

void input_init(InputState *state)
{
    state->left_pressed = 0;
    state->right_pressed = 0;
    state->down_pressed = 0;
    state->up_pressed = 0;
    state->z_pressed = 0;
    state->x_pressed = 0;
    state->space_pressed = 0;
    state->r_pressed = 0;
    state->escape_pressed = 0;
    
    state->left_repeat_timer = 0;
    state->right_repeat_timer = 0;
    state->repeat_interval = ARR;
    state->repeat_delay = DAS;
}

int input_poll(InputState *state, int ch)
{
    if (ch < 0) return 0;
    
    /* Handle scancodes */
    switch (ch) {
        case SC_LEFT:
            state->left_pressed = 1;
            state->left_repeat_timer = 0;
            break;
        case SC_RIGHT:
            state->right_pressed = 1;
            state->right_repeat_timer = 0;
            break;
        case SC_DOWN:
            state->down_pressed = 1;
            break;
        case SC_UP:
            state->up_pressed = 1;
            break;
        case SC_Z:
            state->z_pressed = 1;
            break;
        case SC_X:
            state->x_pressed = 1;
            break;
        case SC_SPACE:
            state->space_pressed = 1;
            break;
        case SC_R:
            state->r_pressed = 1;
            break;
        case SC_ESCAPE:
            state->escape_pressed = 1;
            break;
    }
    
    return ch;
}

int input_is_held(InputState *state, int scancode)
{
    switch (scancode) {
        case SC_LEFT:
            return state->left_pressed;
        case SC_RIGHT:
            return state->right_pressed;
        case SC_DOWN:
            return state->down_pressed;
        case SC_UP:
            return state->up_pressed;
        case SC_Z:
            return state->z_pressed;
        case SC_X:
            return state->x_pressed;
        case SC_SPACE:
            return state->space_pressed;
        case SC_R:
            return state->r_pressed;
        case SC_ESCAPE:
            return state->escape_pressed;
        default:
            return 0;
    }
}

u32 input_get_left_repeat_timer(InputState *state)
{
    return state->left_repeat_timer;
}

u32 input_get_right_repeat_timer(InputState *state)
{
    return state->right_repeat_timer;
}
