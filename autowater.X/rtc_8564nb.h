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

// RTC (real time clock module)
//  - RTC-8564NB
//  - i2c protocol
//  - http://akizukidenshi.com/catalog/g/gI-00233/
//  - Note: This module has the feature of 'day of week',
//          but this code isn't use that.

#ifndef _RTC_8564NB_H_
#define _RTC_8564NB_H_

#ifndef _XTAL_FREQ
// Unless already defined assume 8MHz system frequency
// This definition is required to calibrate __delay_us() and __delay_ms()
#define _XTAL_FREQ 8000000
#endif

//#define FULL_ALARM      // whether using full alarm
//#define USE_CLOCKOUT    // Use CLOCKOUT feature

#ifdef USE_CLOCKOUT
int  rtc_interrupt(void);
int  rtc_init(char inter, char *tm);
#else
int  rtc_init(char *tm);
#endif
void set_ctime(char tm, char s, char *a);
int  rtc_set_time(char *tm);
int  rtc_read_time(char *tm);
void rtc_time_to_string(char *tm, char *c);
void rtc_start_repeated_timer(char clock, char count);
void rtc_stop_repeated_timer(void);
void rtc_set_alarm(char *tm);
void rtc_start_alarm(void);
void rtc_stop_alarm(void);
void rtc_clear_alarm(void);

#endif
