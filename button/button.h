//There are a maximum of 17 GPIO pins, each can be used for a button
#define MAX_BUTTONS 17

//Minimum time in microseconds between pulse changes for pulse to register
#define DEBOUNCE_DELAY 2500

struct button
{
    int pin_number;
    int state;
    int last_state;
    long last_change_time;
    //Callback is called for every button press or release. 
    //Sole parameter is the current button state
    void (*callback)(int); 
};

/*
  Sets up a new button
  'pin' is the number of the GPIO pin to be used (wiringPi numbering)
  'callback' is a function that will be run whenever the button is pressed
  or released. It takes a single integer which is the value of the button
  ('1' for released and '0' for pressed)
*/
struct button *setup_button(int pin, void (*callback)(int));
