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

#include <xc.h>
#include <string.h>
#include "i2c.h"
#include "rtc_8564nb.h"

// define
#define RTC_ADDR   0B1010001    // Slave address of RTC module

// global variables
char rtc_ctrl2;
#ifdef USE_CLOCKOUT
int  rtc_pinmask;
#endif


/**
 * !@brief Convert BCD to number
 *
 * @param[in] dt BCD value
 * @return converted number
 */
unsigned char bcd2bin(unsigned char dt)
{
    //return ((dt >> 4) * 10) + (dt & 0xf);

    // same as below code
    // 10 == 2 + 8 == (1<<1) + (1<<3)
    unsigned char a = (dt >> 4);
    return (a << 1) + (a << 3) + (dt & 0xf);
}

/**
 * !@brief Convert number to BCD value
 *
 * @param[in] number. This value is in the range of 0..99.
 * @return BCD value
 */
unsigned char bin2bcd(unsigned char num)
{
    //return ((num / 10) << 4) | (num%10);

    // same as blow code
    //   x / 10 => (x * 205) >> 11
    //   range of 0..99 is valid
    unsigned short s = num * 205;
    unsigned char a = s >> 11;
    unsigned char b = (a << 1) + (a << 3); // == a * 10
    return (a << 4) | (num - b);
}

/**
 * !@brief Wait 1000ms
 *
 * The reason why it separates to this function is to reduce program size.
 * __delay_ms(XXX) is inline function, so use same parameter is getting bigger.
 */
void delay_1000ms(void)
{
    __delay_ms(1000);
}


/**
 * !@brief Send datetime values
 *
 * @param[in] tm Datetime values.
 */
void send_datetime(char *tm)
{
    //i2c_send(bin2bcd(tm[0]));     // Seconds 0-59
    //i2c_send(bin2bcd(tm[1]));     // Minutes 0-59
    //i2c_send(bin2bcd(tm[2]));     // Hours 0-23
    //i2c_send(bin2bcd(tm[3]));     // Days 1-31
    //i2c_send(bin2bcd(tm[4]));     // Weekdays 0-6 (0 means sunday)
    //i2c_send(bin2bcd(tm[5]));     // Months 1-12
    //i2c_send(bin2bcd(tm[6]));     // Years 00-99

    // same as below
    for (int i=0; i<7; i++) {
        i2c_send(bin2bcd(*tm++));
    }
}

#ifdef USE_CLOCKOUT
/*
 * !@brief Interrupt fucntion for RTC
 *
 * Call this function from global interrupt function in main.c.
 * @return Whether or not interrupted. 1 means interrupt has occured.
 */
int rtc_interrupt(void)
{
    int ret = 0;
    if (IOCIF==1) {
        if ((IOCAF & rtc_pinmask) != 0) {
            ret = 1;
            IOCAF = IOCAF & ~rtc_pinmask;
        }
    }
    return ret;
}
#endif

/**
 * !@brief Initialize RTC module
 *
 * - Disable Alarm, Timer
 * - Enable Repeated Interrupt
 * - CLOCKOUT frequency is 1Hz
 * @param[in] Specify Interrupt pin(CLOCKOUT).
 *            0 means interrupt doesn't use.
 *            1 means RB0 pin .. 8 means RB7 pin.
 * @param[in] tm Date time values.
 *            tm[0] : Seconds 0-59
 *            tm[1] : Minutes 0-59
 *            tm[2] : Hours 0-23
 *            tm[3] : Days 1-31
 *            tm[4] : Weekdays 0-6 (0 means sunday)
 *            tm[5] : Months 1-12
 *            tm[6] : Years 00-99
 * @return Return the result. 0:success 1:failure
 */
#ifdef USE_CLOCKOUT
int rtc_init(char inter, char *tm)
#else
int rtc_init(char *tm)
#endif
{
    char reg1, reg2;
    int ret;

    i2c_init_master();
    delay_1000ms();
    ret = i2c_start(RTC_ADDR, RW_0);
    if (ret != 0) {
        i2c_stop();
        return ret;
    }
    i2c_send(0x01);                     // Set the register address to 01h
    ret = i2c_rstart(RTC_ADDR, RW_1);   // Set repeated start condition
    if (ret != 0) {
        i2c_stop();
        return ret;
    }

    reg1 = i2c_receive(ACK);            // Receive register from 01h
    reg2 = i2c_receive(NOACK);          // Receive register from 02h
    if (reg2 & 0x80) {                  // Initialize when VL bit is setted.
        i2c_rstart(RTC_ADDR, RW_0);
        i2c_send(0x00);                 // Set the register address to 00h
        i2c_send(0x20);                 // Set Control1 (TEST=0,STOP=1)
        rtc_ctrl2 = 0x11;               // Disable interrupt
        i2c_send(rtc_ctrl2);            // Set Control2 register
        send_datetime(tm);
        i2c_send(0x80);                 // Disalbe MinuteAlarm at 09h
        i2c_send(0x80);                 // Disalbe HourAlarm at 0Ah
        i2c_send(0x80);                 // Disable DayAlarm at 0Bh
        i2c_send(0x80);                 // Disalbe WeekDayAlarm at 0Ch
        #ifdef USE_CLOCKOUT
        i2c_send(0x00);                 // Enable CLKOUT at 0Dh
        #else
        i2c_send(0x83);                 // Disable CLOKOUT at 0Dh
        #endif
        i2c_send(0x00);                 // Disable TimerControl at 0Eh
        i2c_send(0x00);                 // Disable Timer at 0Fh

        // Start clock
        i2c_rstart(RTC_ADDR, RW_0);
        i2c_send(0x00);                 // Set the register address to 00h
        i2c_send(0x00);                 // Set Control1 (TEST=0,STOP=0)
        i2c_stop();
        delay_1000ms();
    } else {
        rtc_ctrl2 = reg1;
        i2c_stop();
    }

    #ifdef USE_CLOCKOUT
    // Enable CLOCKOUT pin
    rtc_pinmask = (0x01 << (inter-1));
    if (inter != 0) {
        IOCIE = 1;
        IOCAP = IOCAP | rtc_pinmask;
        IOCAF = 0;
        IOCIF = 0;
    }
    #endif
    return ret;
}

/**
 * !@brief Set the datetime to RTC
 *
 * @param[in] tm Date time values.
 *            tm[0] : Seconds 0-59
 *            tm[1] : Minutes 0-59
 *            tm[2] : Hours 0-23
 *            tm[3] : Days 1-31
 *            tm[4] : Weekdays 0-6 (0 means sunday)
 *            tm[5] : Months 1-12
 *            tm[6] : Years 00-99
 * @return Return the result. 0:success 1:failure
 */
int rtc_set_time(char *tm)
{
    int ret;

    ret = i2c_start(RTC_ADDR, RW_0);
    if (ret == 0) {
        i2c_send(0x00);                 // Set the register address to 00h
        i2c_send(0x20);                 // Set Control1 (TEST=0,STOP=1)

        // Send datetime
        i2c_rstart(RTC_ADDR, RW_0);     // Set repeated start condition
        i2c_send(0x02);                 // Set the register address to 02h
        send_datetime(tm);

        // Start to clock
        i2c_rstart(RTC_ADDR,RW_0);      // Set repeated start condition
        i2c_send(0x00);                 // Set the register address to 02h
        i2c_send(0x00);                 // Set Control1 (TEST=0,STOP=0)
        i2c_stop();
        delay_1000ms();
    } else {
        i2c_stop();
    }
    return ret;
}

/**
 * !@brief Read the datetime from RTC
 *
 * @param[out] tm Date time values.
 *            tm[0] : Seconds 0-59
 *            tm[1] : Minutes 0-59
 *            tm[2] : Hours 0-23
 *            tm[3] : Days 1-31
 *            tm[4] : Weekdays 0-6 (0 means sunday)
 *            tm[5] : Months 1-12
 *            tm[6] : Years 00-99
 * @return Return the result. 0:success 1:failure
 */
int rtc_read_time(char *tm)
{
    const static unsigned char mask[] = {0x7f, 0x7f, 0x3f, 0x3f, 0x07, 0x1f};
    int ret;

    ret = i2c_start(RTC_ADDR,RW_0);
    if (ret == 0) {
        i2c_send(0x02);                     // Set the register address to 02h
        i2c_rstart(RTC_ADDR,RW_1);
        for (int i=0; i<6; i++) {
            *tm = bcd2bin(i2c_receive(ACK) & mask[i]);
            tm++;
        }
        *tm = bcd2bin(i2c_receive(NOACK));  // year
    }
    i2c_stop();
    return ret;
}

/**
 * !@brief Convert datetime value to string
 *
 * @param[in] tm Date value that formatted RTC module value
 * @param[out] c The address of string to output. formatted below.
 *             "yyyymmdd\0hh:mm:ss\0"
 *             e.g. "20110531\013:29:20\0"
 */
void rtc_time_to_string(char *tm, char *c)
{
    char *buf = c;

    //20110531\013:29:20\0
    buf[17] = buf[8] = '\0';
    set_ctime(*tm, ':', &buf[14]);  // sec
    tm++;
    set_ctime(*tm, ':', &buf[11]);  // min
    tm++;
    set_ctime(*tm, ' ', &buf[8]);   // hour
    tm++;
    set_ctime(*tm, '\0', &buf[6]);  // day
    tm++;
    tm++; // skip weekdays
    set_ctime(*tm, '\0', &buf[4]);  // month
    tm++;
    set_ctime(*tm, '0', &buf[1]);   // year
    buf[0] = '2';
}

/**
 * !@brief Convert to one character from BCD value
 *
 * @param[in] tm BCD formatted value
 * @param[in] prefix Set prefix character before output string
 * @param[out] out The address of string to output. formatted below.
 */
void set_ctime(char tm, char prefix, char *out)
{
    tm = bin2bcd(tm);
    if (prefix != '\0') {
        *out = prefix;
        out++;
    }
    *out = (tm >> 4) + 0x30;
    out++;
    *out = (tm & 0x0f) + 0x30;
}

/**
 * !@brief Set repeated timer
 *
 * @param[in] clock Clock interval.
 *                  0:244.14us 1:15.625ms 2:1sec 3:1min
 * @param[in] count Counter value. clock * count => Timer interval.
 */
void rtc_start_repeated_timer(char clock, char count)
{
    i2c_start(RTC_ADDR, RW_0);
    i2c_send(0x0f);               // Set the register address to 0Fh
    i2c_send(count);              // Set Timer(Reg0F) register
    i2c_rstart(RTC_ADDR, RW_0);
    i2c_send(0x0e);               // Set the register address to 0Eh
    i2c_send(clock | 0x80);       // Set TimerControl(Reg0E) register
    i2c_stop();
}

/**
 * !@brief Stop repeated timer.
 */
void rtc_stop_repeated_timer(void)
{
    i2c_start(RTC_ADDR, RW_0);
    i2c_send(0x0e);               // Set the register address to 0Eh
    i2c_send(0x00);               // Clear TimerControl(Reg0E) register
    i2c_rstart(RTC_ADDR, RW_0);
    i2c_send(0x01);               // Set the register address to 0Eh
    rtc_ctrl2 = rtc_ctrl2 & 0xfb; // Clear the timer flag
    i2c_send(rtc_ctrl2);          // Set Control2(Reg01) register
    i2c_stop();
}

/**
 * !@brief Set the alarm
 *
 * @param[in] tm Alarm values. This value depends on FULL_ALARM macro.
 *            FULL_ALARM is defined
 *              tm[0]:minute, tm[1]:hour
 *            FULL_ALARM is not defined
 *              tm[0]:minute, tm[1]:hour, tm[2]:day of month , tm[3]:Weekday
 *            Set to 0x80 to disable value.
 */
void rtc_set_alarm(char *tm)
{
    i2c_start(RTC_ADDR, RW_0);
    i2c_send(0x09);             // Set the register address to 09h
    #ifdef FULL_ALARM
    for (int i=0; i<4; i++)
    #else
    for (int i=0; i<2; i++)
    #endif
    {
        if (*tm != 0xff) {
            i2c_send((char)bin2bcd(*tm));
        } else {
            i2c_send(0x80);     // disable
        }
        tm++;
    }
    #ifndef FULL_ALARM
    i2c_send(0x80); // disable day
    i2c_send(0x80); // disable weekday
    #endif
    rtc_start_alarm();
}

/**
 * !@brief Start the Alarm
 */
void rtc_start_alarm(void)
{
    i2c_rstart(RTC_ADDR, RW_0);
    i2c_send(0x01);               // Set the register address to 01h
    rtc_ctrl2 = (rtc_ctrl2 | 0x02) & 0xf7; // Enable alarm (AIE=1 AF=0)
    i2c_send(rtc_ctrl2);          // Set Control2(Reg01) register
    i2c_stop();
}

/**
 * !@brief Stop the Alarm
 */
void rtc_stop_alarm(void)
{
    i2c_start(RTC_ADDR, RW_0);
    i2c_send(0x01);                 // Set the register address to 01h
    rtc_ctrl2 = rtc_ctrl2 & 0xf5;   // Disable alarm(AIE=0 AF=0)
    i2c_send(rtc_ctrl2);            // Set Control2(Reg01) register
    i2c_stop();
}

/**
 * !@brief Clear alarm flag
 *
 * But Alaam continue working.
 */
void rtc_clear_alarm(void)
{
    i2c_start(RTC_ADDR, RW_0);
    i2c_send(0x01);                 // Set the register address to 01h
    rtc_ctrl2 = rtc_ctrl2 & 0xf7;   // Clear Alarm Flag (AF=0)
    i2c_send(rtc_ctrl2);            // Set Control2(Reg01) register
    i2c_stop();
}
