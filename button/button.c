#include <wiringPi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>

#include "button.h"

struct button buttons[MAX_BUTTONS];
int numberofbuttons = 0;
int buttons_thread_running = 0;

long micro_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000000 + (tv.tv_usec);
}

PI_THREAD (buttons_thread)
{
    struct button *button;
    while(1)
    {
        for(button = buttons; button < buttons + numberofbuttons; button++)
        {
            long ntime = micro_time();
            int pin_state = digitalRead(button->pin_number);

            if(pin_state != button->last_state)
                button->last_change_time = ntime;
            if((ntime - button->last_change_time) > DEBOUNCE_DELAY)
            {
                if(pin_state != button->state)
                    (*(button->callback))(pin_state);
                button->state = pin_state;
            }
            button->last_state = pin_state;
        }
        delay(25);
    }
}

struct button *setup_button(int pin, void (*callback)(int))
{
    if (numberofbuttons > MAX_BUTTONS)
    {
        printf("Maximum number of buttons exceded: %i\n", MAX_BUTTONS);
        return NULL;
    }

    struct button *newbutton = buttons + numberofbuttons++;
    newbutton->pin_number = pin;
    newbutton->state = 1;
    newbutton->last_change_time = micro_time();
    newbutton->callback = callback;

    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_UP);
    if(buttons_thread_running == 0)
    {
        int err = piThreadCreate (buttons_thread) ;
        if (err != 0)
            printf ("Failed to start buttons thread.\n");
        else
            buttons_thread_running = 1;
    }

    return newbutton;
}
