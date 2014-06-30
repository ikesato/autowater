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

#ifndef _button_h_
#define _button_h_

#include <GenericTypeDefs.h>

// Enable this define if used log pressed
//#define USE_VERY_LONG_PRESSED

void button_init(BYTE button_bit_mask);
void button_proc_every_main_loop(BYTE button_port);
void button_proc_every_timer_interrupt(void);

/**
 * Idle counter that increments timer interrupt.
 */
extern WORD button_idle_timer;

/**
 * State of button
 * bits means buttons bits whether pressed or upped.
 * Return 1 means pressed and 0 means upped.
 */
extern BYTE button_state;

/**
 * Whether upped state of button
 * This value is avaiable one frame.
 * Return bits means buttons bits whether pressed or upped.
 * 1 means upped and 0 means not.
 */
extern BYTE button_upped_state;

/**
 * Whether pressed state of button
 * This value is avaiable one frame.
 * Return bits means buttons bits whether pressed or upped.
 * 1 means pressed and 0 means not.
 */
extern BYTE button_pressed_state;

/**
 * Long pressed state of button
 * This value is avaiable one frame.
 * Return bits means buttons bits whether pressed or upped.
 * 1 means long pressed and 0 means not.
 */
extern BYTE button_long_pressed_state;

/**
 * Keep long pressed state of button
 * This value is avaiable one frame.
 * Return bits means buttons bits whether pressed or upped.
 * 1 means keep long pressed and 0 means not.
 */
extern BYTE button_keep_long_pressed_state;


#ifdef USE_VERY_LONG_PRESSED
/**
 * Very long pressed state of button
 * This value is avaiable one frame.
 * Return bits means buttons bits whether pressed or upped.
 * 1 means very long pressed and 0 means not.
 */
extern BYTE button_very_long_pressed_state;
#endif


#endif//_button_h_
