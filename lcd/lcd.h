#define LCD_WIDTH 16
#define LCD_CHR 1
#define LCD_CMD 0
#define LCD_LINE_1 0x80
#define LCD_LINE_2 0xC0

#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

#define E_PULSE 500
#define E_DELAY 500

struct lcd
{
    int pin_RS;
    int pin_E;
    int pin_D4;
    int pin_D5;
    int pin_D6;
    int pin_D7;
};


/*
  Write a byte to an LCD, 
  set mode to LCD_CHR and this will print a character
  set mode to LCD_CMD and this will send a command to the LCD
*/
void lcd_byte(struct lcd lcd, char byte, int mode);

//Write a string to an LCD
void lcd_string(struct lcd lcd, const char *message,int line);

//Write a string to an LCD, wrapping it to the next line if it is too long or 
//contains a newline
void lcd_wrapped_string(struct lcd lcd, const char *message);

//Write a string to an LCD, causing it to scroll if it is too long
void lcd_marquee (struct lcd lcd, const char *message, int line);

//Register a new LCD with the pins given and initialize the device
struct lcd lcd_init(int pin_RS, int pin_E, int pin_D4, int pin_D5, int pin_D6, int pin_D7);
