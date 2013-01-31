#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringPi.h>

#include "lcd.h"

void lcd_byte(struct lcd lcd, char byte, int mode)
{
    digitalWrite(lcd.pin_RS, mode);
    
    digitalWrite(lcd.pin_D4, 0);
    digitalWrite(lcd.pin_D5, 0);
    digitalWrite(lcd.pin_D6, 0);
    digitalWrite(lcd.pin_D7, 0);
    if((byte&0x10) == 0x10)
        digitalWrite(lcd.pin_D4, 1);
    if((byte&0x20) == 0x20)
        digitalWrite(lcd.pin_D5, 1);
    if((byte&0x40) == 0x40)
        digitalWrite(lcd.pin_D6, 1);
    if((byte&0x80) == 0x80)
        digitalWrite(lcd.pin_D7, 1);

    usleep(E_DELAY);
    digitalWrite(lcd.pin_E, 1);
    usleep(E_PULSE);
    digitalWrite(lcd.pin_E, 0);
    usleep(E_DELAY);
    
    digitalWrite(lcd.pin_D4, 0);
    digitalWrite(lcd.pin_D5, 0);
    digitalWrite(lcd.pin_D6, 0);
    digitalWrite(lcd.pin_D7, 0);
    if((byte&0x01) == 0x01)
        digitalWrite(lcd.pin_D4, 1);
    if((byte&0x02) == 0x02)
        digitalWrite(lcd.pin_D5, 1);
    if((byte&0x04) == 0x04)
        digitalWrite(lcd.pin_D6, 1);
    if((byte&0x08) == 0x08)
        digitalWrite(lcd.pin_D7, 1);

    usleep(E_DELAY);
    digitalWrite(lcd.pin_E, 1);
    usleep(E_PULSE);
    digitalWrite(lcd.pin_E, 0);
    usleep(E_DELAY);
}

void lcd_string(struct lcd lcd, char *message,int line)
{
    if (line == 1)
        lcd_byte(lcd, LCD_LINE_1, LCD_CMD);
    else if (line == 2)
        lcd_byte(lcd, LCD_LINE_2, LCD_CMD);
    else
        printf("error: bad line number: %i", line);

    int idx = 0;
    for(;message[idx] != '\0' && idx < LCD_WIDTH; idx++)
        lcd_byte(lcd, message[idx],LCD_CHR);
    for(;idx < LCD_WIDTH; idx++)
        lcd_byte(lcd, ' ',LCD_CHR);
}

void lcd_wrapped_string(struct lcd lcd, char *message)
{
    lcd_byte(lcd, LCD_LINE_1, LCD_CMD);
    int idx = 0;
    for(;idx<LCD_WIDTH-1 && message[idx] != '\n' && message[idx] != '\0';idx++)
        lcd_byte(lcd, message[idx],LCD_CHR);

    if(idx==LCD_WIDTH-1)
        lcd_byte(lcd, '-',LCD_CHR);
    else
        idx++;

    if(strlen(message) > idx+1)
        lcd_string(lcd, message+idx, 2);
}

void lcd_marquee (struct lcd lcd, char *message, int line)
{
    int overflow = strlen(message) - LCD_WIDTH;
    if(overflow <= 0)
    {
        lcd_string(lcd, message, line);
        return;
    }

    int idx = 0;
    for(; idx <= overflow; idx++)
    {
        lcd_string(lcd, message+idx,line);
        usleep(350000);
    }
    usleep(500000);
    for(--idx; idx >= 0; idx--)
    {
        lcd_string(lcd, message+idx,line);
        usleep(350000);
    }
}

struct lcd lcd_init(int pin_RS, int pin_E, int pin_D4, int pin_D5, int pin_D6, int pin_D7)
{
    pinMode(pin_RS, OUTPUT);
    pinMode(pin_E, OUTPUT);
    pinMode(pin_D4, OUTPUT);
    pinMode(pin_D5, OUTPUT);
    pinMode(pin_D6, OUTPUT);
    pinMode(pin_D7, OUTPUT);

    struct lcd lcd;
    lcd.pin_RS = pin_RS;
    lcd.pin_E = pin_E;
    lcd.pin_D4 = pin_D4;
    lcd.pin_D5 = pin_D5;
    lcd.pin_D6 = pin_D6;
    lcd.pin_D7 = pin_D7;

    lcd_byte(lcd, 0x33, LCD_CMD);
    lcd_byte(lcd, 0x32, LCD_CMD);
    lcd_byte(lcd, 0x28, LCD_CMD);
    lcd_byte(lcd, 0x0C, LCD_CMD);
    lcd_byte(lcd, 0x06, LCD_CMD);

    lcd_byte(lcd, LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT, LCD_CMD);
    lcd_byte(lcd, LCD_CLEARDISPLAY, LCD_CMD);

    return lcd;
}
