#define LED_COLOR_RED 0
#define LED_COLOR_GREEN 1
#define LED_COLOR_BLUE 2
#define LED_COLOR_YELLOW 3
#define LED_COLOR_MAGENTA 4
#define LED_COLOR_CYAN 5
#define LED_COLOR_WHITE 6
#define LED_COLOR_OFF 7

struct led
{
    int pin_red;
    int pin_green;
    int pin_blue;
};

void light_led(struct led led, int red, int green, int blue);

void light_led_color(struct led led, int color);

struct led led_init(int pin_red, int pin_green, int pin_blue);

