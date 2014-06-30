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

// LCD
//  - AQM0802A
//  - 8x2 LCD display
//  - i2c protocol
//  - http://akizukidenshi.com/catalog/g/gP-06669/

#include <xc.h>
#include "i2c.h"
#include "lcd_aqm0802a.h"

#define LCD_ADDR 0x3E       // i2c address

/**
 * !@brief Send command to lcd
 *
 * @param[in] c Command code
 */
static void lcd_command(unsigned char c)
{
    int  ret;

    ret = i2c_start(LCD_ADDR, RW_0);
    if (ret == 0) {
        i2c_send(0b10000000);
        i2c_send(c);
    }
    i2c_stop();
    __delay_us(26);
}

/**
 * !@brief Initialize LCD
 */
void lcd_init(void)
{
    i2c_init_master();

    __delay_ms(40);         // Wait 40ms after power on
    lcd_command(0x38);      // Function set : 8bits, 2lines
    lcd_command(0x39);      // Function set : select instruction table
    lcd_command(0x14);      // Internal OSC frequency
    lcd_command(0x70);      // Contrast set
    lcd_command(0x56);      // Power/ICON/Contrast control
    lcd_command(0x6C);      // Follower control
    __delay_ms(200);        // Wait 200ms
    lcd_command(0x38);      // Function set : disalbe instruction table
    lcd_command(0x0c);      // Display ON : Curosr OFF, Blink OFF
    lcd_command(0x06);      // Entry mode set : move cursor to right after put character
    lcd_clear();            // Clear Display
}

/**
 * !@brief Clear display
 */
void lcd_clear(void)
{
    lcd_command(0x01);      // Fill 20h, cursor to (0,0)
    __delay_us(1100);       // wait 1.08ms
}

/**
 * !@brief Set the cursor position
 *
 * @param[in] col Position of horizontal. Range => 0..7
 * @param[in] row Position of vertical. Range => 0..1
 */
void lcd_set_cursor(char col, char row)
{
    int row_offsets[] = {0x00, 0x40};
    lcd_command(0x80 | (col + row_offsets[row]));
}

/**
 * !@brief Show and move the cursor
 *
 * @param[in] col Position of horizontal. Range => 0..7
 * @param[in] row Position of vertical. Range => 0..1
 */
void lcd_show_cursor(char col, char row)
{
    lcd_set_cursor(col, row);
    lcd_command(0x0c | 0x1);
}

/**
 * !@brief Hide the cursor
 */
void lcd_hide_cursor(void)
{
    lcd_command(0x0c);
}

/**
 * !@brief Put the character
 *
 * @param[in] c Character to put
 */
void lcd_putc(char c)
{
    int  ret;

    ret = i2c_start(LCD_ADDR, RW_0);
    if (ret == 0) {
        i2c_send(0b11000000);   // send control byte
        i2c_send(c);
    }
    i2c_stop();
    __delay_us(26);
}

/**
 * !@brief Put the string
 *
 * @param[in] s Address of string to put
 */
void lcd_puts(const char * s)
{
    int  ret;

    ret = i2c_start(LCD_ADDR, RW_0);
    if (ret == 0) {
        i2c_send(0b01000000);   // send control byte
        while(*s) {
            i2c_send(*s++);
            __delay_us(26);
        }
    }
    i2c_stop();
}

/**
 * !@brief register font image of character
 *
 * @param[in] Address of font
 * @param[in] Buffer of image
 */
void lcd_create_char(char p, char *dt)
{
    int ret, i;

    ret = i2c_start(LCD_ADDR, RW_0);
    if (ret == 0) {
        // Set the address
        i2c_send(0b10000000);   // send control byte
        i2c_send(0x40 | (p << 3));
        __delay_us(26);

        // Register image
        i2c_send(0b01000000);   // send control byte
        for (i=0; i < 7; i++) {
            i2c_send(*dt++);
            __delay_us(26);
        }
    }
    i2c_stop();
}
