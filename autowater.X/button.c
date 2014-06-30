/******************************************************************************
 * Copyright (c) 2014 Satoshi Ikeda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

#include "button.h"

// Timer overflow frequency [us]
#define FREQ 32256L
// Long press interval [timer frequency] 1000 means 1[msec]
// Note: this macro is range of BYTE
#define LONG_PRESSED_INTERVAL        ((BYTE)(1000L * 1000L / FREQ))
// Very long press interval [timer frequency] 3000 means 3[msec]
// Note: this macro is range of BYTE
#define VERY_LONG_PRESSED_INTERVAL   ((BYTE)(3000L * 1000L / FREQ))

// Button state 0:not pressed 1:pressed
BYTE button_state;
// Used button mask
BYTE button_mask;
// Button upped state 1:upped 0:other (Available one frame)
BYTE button_upped_state;
// Button pressed state 1:pressed 0:other (Available one frame)
BYTE button_pressed_state;
// Long pressed state 1:pressed 0:other (Available one frame)
BYTE button_long_pressed_state;
// Keep long pressed state 1:pressed 0:other
BYTE button_keep_long_pressed_state;

#ifdef USE_VERY_LONG_PRESSED
// Very long pressed state 1:pressed 0:other (Available one frame)
BYTE button_very_long_pressed_state;
// Keep very long pressed state 1:pressed 0:other
BYTE button_keep_very_long_pressed_state;
#endif

// Timer counter
BYTE button_timer = 0;
// Idle counter that increments timer interrupt.
WORD button_idle_timer = 0;

/**
 * Init button
 * Params:
 *   'mask' specifies the used buttons.
 *   0b0011 means used RA0 and RA1 buttons.
 */
void button_init(BYTE mask)
{
    button_mask = mask;
    button_state = 0;
    button_upped_state = 0;
    button_pressed_state = 0;
    button_long_pressed_state = 0;
    button_keep_long_pressed_state = 0;
#ifdef USE_VERY_LONG_PRESSED
    button_very_long_pressed_state = 0;
    button_keep_very_long_pressed_state = 0;
#endif
    button_timer = 0;
}

/**
 * !@brief Call this function every main loop
 *
 * @param[in] button_port Specifies PORTA or PORTB or ...
 */
void button_proc_every_main_loop(BYTE button_port)
{
    BYTE last_state = button_state;
    BYTE xor_state;
    BYTE changed;

    button_state = (~button_port) & button_mask;
    xor_state = button_state ^ last_state;
    button_pressed_state = xor_state & button_state;
    button_upped_state = xor_state & (~button_state);

    if (button_state == 0) {
        if (button_upped_state != 0) {
            button_idle_timer = 0;
        }
        button_timer = 0;
        button_long_pressed_state = 0;
        button_keep_long_pressed_state = 0;
#ifdef USE_VERY_LONG_PRESSED
        button_very_long_pressed_state = 0;
        button_keep_very_long_pressed = 0;
    } else if (button_timer >= VERY_LONG_PRESSED_INTERVAL) {
        if (button_keep_very_long_pressed == 0) {
            button_keep_very_long_pressed_state = button_state;
            button_very_long_pressed_state = button_state;
        } else {
            button_very_long_pressed_state = 0;
        }
#endif
    } else if (button_timer >= LONG_PRESSED_INTERVAL) {
        if (button_keep_long_pressed_state == 0) {
            button_keep_long_pressed_state = button_state;;
            button_long_pressed_state = button_state;
        } else {
            button_long_pressed_state = 0;
        }
    } else {
        button_long_pressed_state = 0;
        button_keep_long_pressed_state = 0;
#ifdef USE_VERY_LONG_PRESSED
        button_very_long_pressed_state = 0;
        button_keep_very_long_pressed_state = 0;
#endif
    }
}

/**
 * !@brief Call this function every timer interrupt
 */
void button_proc_every_timer_interrupt(void)
{
    if (button_timer < 0xff) {
        button_timer++;
    }
    button_idle_timer++;
}
