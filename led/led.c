#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>

#include "led.h"

void light_led(struct led led, int red, int green, int blue)
{
    digitalWrite(led.pin_red, red);
    digitalWrite(led.pin_green, green);
    digitalWrite(led.pin_blue, blue);
}

void light_led_color(struct led led, int color)
{
    switch(color)
	{
	case LED_COLOR_RED:
	    light_led(led, 1, 0, 0);
	    break;
	case LED_COLOR_GREEN:
	    light_led(led, 0, 1, 0);
	    break;
	case LED_COLOR_BLUE:
	    light_led(led, 0, 0, 1);
	    break;
	case LED_COLOR_YELLOW:
	    light_led(led, 1, 1, 0);
	    break;
	case LED_COLOR_MAGENTA:
	    light_led(led, 1, 0, 1);
	    break;
	case LED_COLOR_CYAN:
	    light_led(led, 0, 1, 1);
	    break;
	case LED_COLOR_WHITE:
	    light_led(led, 1, 1, 1);
	    break;
	case LED_COLOR_OFF:
	default:
	    light_led(led, 0, 0, 0);
	    break;
	}
}

struct led led_init(int pin_red, int pin_green, int pin_blue)
{
    pinMode(pin_red, OUTPUT);
    pinMode(pin_green, OUTPUT);
    pinMode(pin_blue, OUTPUT);

    digitalWrite(pin_red, 0);
    digitalWrite(pin_green, 0);
    digitalWrite(pin_blue, 0);

    struct led led;
    led.pin_red = pin_red;
    led.pin_green = pin_green;
    led.pin_blue = pin_blue;

    return led;
}
