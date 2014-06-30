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

// Processor : PIC 12F1822
// Compiler  : MPLAB(R) XC8 C Compiler Version 1.30

#include <xc.h>
#include "i2c.h"
#include "lcd_aqm0802a.h"
#include "rtc_8564nb.h"
#include "button.h"

// Setting configuration1
// Data Memory Code Protection
#pragma config CPD = OFF
// Brown-out Reset Enable (OFF,ON,NSLEEP,SBODEN)
#pragma config BOREN = NSLEEP
// Internal/External Switchover
#pragma config IESO = OFF
// Oscillator Selection (ECM,XT,LP,EXTRC,ECH,ECL,INTOSC,HC)
#pragma config FOSC = INTOSC
// Fail-Safe Clock Monitor Enable
#pragma config FCMEN = OFF
// MCLR Pin Function Select
#pragma config MCLRE = OFF
// Watchdog Timer Enable (OFF,ON,NSLEEP,SWDTEN)
#pragma config WDTE = OFF
// Flash Program Memory Code Protection
#pragma config CP = OFF
// Power-up Timer Enable
#pragma config PWRTE = ON
// Clock Out Enable
#pragma config CLKOUTEN = OFF

// Setting configuration2
// PLL Enable
#pragma config PLLEN = OFF
// Flash Memory Self-Write Protection (OFF,BOOT,HALF,ALL)
#pragma config WRT = OFF
// Stack Overflow/Underflow Reset Enable
#pragma config STVREN = ON
// Brown-out Reset Voltage Selection
#pragma config BORV = HI
// Low-Voltage Programming Enable
#pragma config LVP = OFF


// Defines
#define RTCINTPIN             (1<<5)    // RA5 RTC-INT interrupt
#define SW1                   (1<<4)    // RA4 is switch 1
#define SW2                   (1<<3)    // RA3 is switch 2
#define RELAY                 RA0       // RA0 is relay port
#define RELAY_BIT             (1<<0)    // RA0 is relay port (bit)

#define T0CNT                 4         // First Value of timer0 (4 is most close for generating 1 second)
#define FREQ                  (32256L)  // Timer overflow frequency [us]
#define ONE_SEC               ((WORD)(1000L * 1000L / FREQ))  // one second of WORD value
#define SLEEPING_TIME         ((WORD)(60000L * 1000L / FREQ)) // sleep time

#define SHOW_CLOCK            0
#define SET_CLOCK_DATE_YEAR   1
#define SET_CLOCK_DATE_MONTH  2
#define SET_CLOCK_DATE_DAY    3
#define SET_CLOCK_TIME_HOUR   4
#define SET_CLOCK_TIME_MIN    5
#define SET_CLOCK_TIME_SEC    6
#define SHOW_ALARM            7
#define SET_USE_ALARM         8
#define SET_ALARM_HOUR        9
#define SET_ALARM_MIN         10
#define SHOW_PON_TIME         11
#define SET_PON_TIME          12

// Global values
unsigned char use_alarm;            // preference: whether alarm used
unsigned char alarm_time[2];        // preference: minute and hour
unsigned char poweron_time = 10;    // preference: power on interval [sec]
unsigned char mode;                 // mode
unsigned char setting_value;        // current setting value
unsigned char current_time[7];      // current time
char buf[18];                       // temporary buffer
unsigned char interrupted_alarm = 0;// flag of interrupted alarm
unsigned short poweron_remain;      // remain time for power on
const char *on_off_str[] = {"OFF", "ON "};

// Define struct
typedef struct {
    const char *title;  // title
    char min;           // min value
    char max;           // max value
    char ct_pos;        // position of current_time
    char buf_pos;       // position of buf
    char cursor_pos;    // positoin of cursor
    char next_mode;     // next setting mode
    char next_ct_pos;   // positoin of next current_time
} set_clock_t;

typedef struct {
    char at_pos;        // position of alarm_time
    char max;           // max value
    char cursor_pos;    // positoin of cursor
    char next_mode;     // next setting mode
    char next_at_pos;   // positoin of next alarm_time
} set_alarm_time_t;

// Prototypes
void show_clock(void);
void set_clock(char set_pos);
void show_alarm(void);
void make_alarm_str(void);
void set_use_alarm(void);
void set_alarm_time(char set_pos);
void display(const char *first_line, char *second_line);
void show_cursor(char cursor_pos);
void show_pon_time(void);
void set_pon_time(void);
void make_pon_str(void);

/**
 * !@brief Interrupt function
 */
void interrupt interrupt_func(void)
{
    // timer interrupt
    if (T0IF == 1) {
        #ifdef T0CNT
        TMR0 = T0CNT;
        #endif
        T0IF = 0;
        poweron_remain--;
        if (poweron_remain == 0) {
            RELAY = 0;
        }
        button_proc_every_timer_interrupt();
    }

    // I2C interrupt handler
    i2c_interrupt();

    // alarm interrupt from RTC
    if (IOCIF == 1) {
        if ((IOCAF & RTCINTPIN) != 0) {
            interrupted_alarm = 1;
        }
        IOCAF = 0;
    }

    // Clear Interrupt-on-Change Interrupt Flag bit
    IOCIF = 0;
}


/**
 * !@brief Initialize function
 */
void init(void)
{
    // 0       1       2     3    4                  5      6
    // second, minute, hour, day, weekday(not used), month, year
    char start_clock[] = {0,0,0,17,0,3,14};

    OSCCON     = 0b01110010;    // bit3-6: Internal clock => 8MHz
                                // bit1: Use internal clock
    OPTION_REG = 0b00000111;    // bit7:Weak Pull-up Enable bit => ON
                                // bit0-2: Prescaler rate => 1:256
    ANSELA     = 0b00000000;    // Not use alalog select register
    TRISA      = 0b00111110;    // Input: RA1(SCL)/RA2(SDA), RA3(INT), RA4/RA5(switch)
                                // Output: RA0(relay)
    WPUA       = 0b00111110;    // Using Pull-up: RA1,RA2,RA3,RA4,RA5
    PORTA      = 0b00000000;    // Initialize all GPIO to LO

    IOCIE = 1;                  // Enable Interrupt-on-Change bit
    IOCAN = IOCAN | RTCINTPIN | SW1 | SW2; // Interrupt-on-Change Negative Edge ports

    // Initialize timer
    #ifdef T0CNT
    TMR0 = T0CNT;
    #else
    TMR0 = 0;
    #endif
    TMR0IF = 0;
    TMR0IE = 1;
    GIE = 0;    // GIE is disable yet. But enabled later.

    // Initialize button library
    button_init(SW1|SW2);

    // Initialize LCD
    lcd_init();
    lcd_set_cursor(0, 0);
    lcd_puts("Hello");

    // Initialize RTC
    rtc_init(start_clock);
}

/**
 * !@brief Loop function
 */
void loop(void)
{
    mode = 0;
    while(1) {
        button_proc_every_main_loop(PORTA);
        if (mode == SHOW_CLOCK) {
            show_clock();
        } else if (SET_CLOCK_DATE_YEAR <= mode && mode <= SET_CLOCK_TIME_SEC) {
            set_clock(mode - SET_CLOCK_DATE_YEAR);
        } else if (mode == SHOW_ALARM) {
            show_alarm();
        } else if (mode == SET_USE_ALARM) {
            set_use_alarm();
        } else if (SET_ALARM_HOUR <= mode && mode <= SET_ALARM_MIN) {
            set_alarm_time(mode - SET_ALARM_HOUR);
        } else if (mode == SHOW_PON_TIME) {
            show_pon_time();
        } else if (mode == SET_PON_TIME) {
            set_pon_time();
        }

        if (interrupted_alarm) {
            // If this code placed in interrupt function then program size is too bigger.
            // (rtc_start_alarm code was dulilicated.)
            // So this codes placed here.
            interrupted_alarm = 0;
            poweron_remain = ONE_SEC * (WORD)poweron_time;
            RELAY = 1;
            rtc_start_alarm();
        }
        if (button_idle_timer > SLEEPING_TIME) {
            // go to sleep
            PORTA = 0b00000000;
            lcd_clear();
            SLEEP();

            // wake up here
            button_proc_every_main_loop(PORTA); // avoid to press button
            button_idle_timer = 0;
            mode = SHOW_CLOCK;
        }
        if (RELAY != 0) {
            button_idle_timer = 0;
        }
        __delay_ms(50);
    }
}


/**
 * !@brief Main function
 */
void main(void)
{
    // Divided to function to reduce stack size.
    init();
    loop();
}

/**
 * !@brief Button press function for showing mode
 */
void press_proc_for_showing(char next_mode, char next_set_mode, unsigned char value)
{
    if (button_pressed_state & SW1) {
        mode = next_mode;
        lcd_clear();
    } else if (button_long_pressed_state & SW2) {
        mode = next_set_mode;
        setting_value = value;
        lcd_clear();
    }
}

/**
 * !@brief Button press function for setting mode
 */
void press_proc_for_setting(char next_mode, char next_value)
{
    if (button_pressed_state & SW2) {
        mode = next_mode;
        setting_value = next_value;
        lcd_clear();
    }
}

/**
 * !@brief Show clock function
 */
void show_clock(void)
{
    // Read the datetime from RTC module
    rtc_read_time(current_time);
    rtc_time_to_string(current_time, buf);

    display(&buf[0], &buf[9]);
    press_proc_for_showing(SHOW_ALARM, SET_CLOCK_DATE_YEAR, current_time[6]); // 6 means year
}

/**
 * !@brief Display lines to LCD display
 */
void display(const char *first_line, char *second_line)
{
    lcd_set_cursor(0, 0);
    lcd_puts(first_line);
    lcd_set_cursor(0, 1);
    lcd_puts(second_line);
    lcd_hide_cursor();
}

/**
 * !@brief Show cursor in LCD
 */
void show_cursor(char cursor_pos)
{
    lcd_show_cursor(cursor_pos, 1);
}

unsigned char choose_value(unsigned char min, unsigned char max)
{
    if ((button_pressed_state | button_keep_long_pressed_state) & SW1) {
        setting_value++;
    }
    if (setting_value > max) {
        setting_value = min;
    }
    return setting_value;
}

void set_clock(char set_pos)
{
    const static set_clock_t clock_datas[] = {
        //title
        //|       min
        //|       |  max
        //|       |  |   ct_pos
        //|       |  |   |  buf_pos
        //|       |  |   |  |  cursor_pos
        //|       |  |   |  |  |  next_mode
        //|       |  |   |  |  |  |                     next_ct_pos
        {"DATE?", 0, 99, 6, 0, 3, SET_CLOCK_DATE_MONTH, 5},
        {"DATE?", 1, 12, 5, 0, 5, SET_CLOCK_DATE_DAY,   3},
        {"DATE?", 1, 31, 3, 0, 7, SET_CLOCK_TIME_HOUR,  2},
        {"TIME?", 0, 23, 2, 9, 1, SET_CLOCK_TIME_MIN,   1},
        {"TIME?", 0, 59, 1, 9, 4, SET_CLOCK_TIME_SEC,   0},
        {"TIME?", 0, 59, 0, 9, 7, SHOW_CLOCK,           0},
    };
    const set_clock_t *cd = &clock_datas[set_pos];
    current_time[cd->ct_pos] = choose_value(cd->min, cd->max);
    rtc_time_to_string(current_time, buf);
    display(cd->title, &buf[cd->buf_pos]);
    show_cursor(cd->cursor_pos);
    press_proc_for_setting(cd->next_mode, current_time[cd->next_ct_pos]);
    if (mode == SHOW_CLOCK) {
        rtc_set_time(current_time);
    }
}

void show_alarm(void)
{
    char *pt;
    if (use_alarm != 0) {
        make_alarm_str();
        pt = buf;
    } else {
        pt = (char*)on_off_str[0];
    }
    display("ALARM", pt);
    press_proc_for_showing(SHOW_PON_TIME, SET_USE_ALARM, use_alarm);
}

void make_alarm_str(void)
{
    set_ctime(alarm_time[1], '\0', &buf[0]);  // hour
    set_ctime(alarm_time[0], ':',  &buf[2]);  // min
    buf[5] = '\0';
}

void set_use_alarm(void) {
    use_alarm = choose_value(0, 1);
    display("ALARM?", (char*)on_off_str[use_alarm]);
    show_cursor(0);
    if (button_pressed_state & SW2) {
        if (use_alarm) {
            mode = SET_ALARM_HOUR;
            setting_value = alarm_time[1];
        } else {
            mode = SHOW_ALARM;
            rtc_stop_alarm();
        }
        lcd_clear();
    }
}

void set_alarm_time(char set_pos) {
    const static set_alarm_time_t alarm_time_datas[] = {
        //at_pos
        //|  max
        //|  |   cursor_pos
        //|  |   |  next_mode
        //|  |   |  |              next_at_pos
        { 1, 23, 1, SET_ALARM_MIN, 0}, // SET_ALARM_HOUR
        { 0, 59, 4, SHOW_ALARM,    0}, // SET_ALARM_MIN
    };
    const set_alarm_time_t *ad = &alarm_time_datas[set_pos];
    alarm_time[ad->at_pos] = choose_value(0, ad->max);
    make_alarm_str();
    display("ALARM?", buf);
    show_cursor(ad->cursor_pos);
    press_proc_for_setting(ad->next_mode, alarm_time[ad->next_at_pos]);
    if (mode == SHOW_ALARM) {
        rtc_set_alarm(alarm_time);
    }
}


void show_pon_time(void)
{
    make_pon_str();
    display("PON", buf);
    press_proc_for_showing(SHOW_CLOCK, SET_PON_TIME, poweron_time);
}

void set_pon_time(void)
{
    poweron_time = choose_value(1, 99);
    make_pon_str();
    display("PON?", buf);
    show_cursor(1);
    press_proc_for_setting(SHOW_PON_TIME, 0);
}

void make_pon_str(void)
{
    set_ctime(poweron_time, '\0', buf);
    buf[2] = 's';
    buf[3] = 'e';
    buf[4] = 'c';
    buf[5] = '\0';
}
