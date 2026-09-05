#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

/* Key scancodes */
#define SC_LEFT 0x25
#define SC_RIGHT 0x27
#define SC_DOWN 0x28
#define SC_UP 0x26
#define SC_Z 0x32
#define SC_X 0x2E
#define SC_SPACE 0x39
#define SC_R 0x31
#define SC_ESCAPE 0x01

/* Input state */
typedef struct {
    u8 left_pressed;
    u8 right_pressed;
    u8 down_pressed;
    u8 up_pressed;
    u8 z_pressed;
    u8 x_pressed;
    u8 space_pressed;
    u8 r_pressed;
    u8 escape_pressed;
    
    /* DAS/ARR timers for held movement */
    u32 left_repeat_timer;
    u32 right_repeat_timer;
    u32 repeat_interval;
    u32 repeat_delay;
} InputState;

/* Initialize input state */
void input_init(InputState *state);

/* Process keyboard input, returns 1 if key pressed */
int input_poll(InputState *state, int ch);

/* Check if key is currently held down */
int input_is_held(InputState *state, int scancode);

/* Get repeat timer for left movement */
u32 input_get_left_repeat_timer(InputState *state);

/* Get repeat timer for right movement */
u32 input_get_right_repeat_timer(InputState *state);

#endif /* INPUT_H */
