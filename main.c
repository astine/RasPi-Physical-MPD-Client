#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <alsa/asoundlib.h>

#include "rotaryencoder/rotaryencoder.h"
#include "lcd/lcd.h"

struct encoder encoder;

static char card[64] = "default";
snd_mixer_t *handle= NULL;

void sig_handler(int signo)
{
    if(signo == SIGINT)
    {
        printf("SIGINT recieved\n");
        if(handle != NULL)
            snd_mixer_close(handle);
        exit(1);
    }
}

void print_vol_bar(snd_mixer_elem_t *elem,struct lcd lcd)
{
    long min, max, vol;
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_get_playback_volume(elem,0,&vol);

    int volp = rint(((double)(vol - min) / (double)(max - min)) * 16);
    char volbar[LCD_WIDTH];

    int idx = 0;
    for(idx;idx<volp;idx++)
        volbar[idx] = '=';
    for(idx;idx<LCD_WIDTH;idx++)
        volbar[idx] = '_';

    lcd_string(lcd,volbar,2);
}

void main()
{
    printf("Starting...\n");
    
    if(signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\n Can't catch SIGINT. \n");

    wiringPiSetup();
    piHiPri(99);
    struct encoder *encoder = setupencoder(7,1);
    if(encoder == NULL) { exit(1); }
    int oldvalue = encoder->value;

    struct lcd lcd = lcd_init(6, 5, 4, 0, 2, 3);

    snd_mixer_t *handle= NULL;
    snd_mixer_elem_t *elem;

    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid,0);
    snd_mixer_selem_id_set_name(sid,"PCM");

    int err = 0;
    if(snd_mixer_open(&handle, 0) < 0)
    {
        printf("Error openning mixer");
        exit(1);
    }
    if(snd_mixer_attach(handle, card) < 0)
    {
        printf("Error attaching mixer");
        snd_mixer_close(handle);
        exit(1);
    }
    if(snd_mixer_selem_register(handle, NULL, NULL) < 0)
    {
        printf("Error registering mixer");
        snd_mixer_close(handle);
        exit(1);
    }
    if(snd_mixer_load(handle) < 0)
    {
        printf("Error loading mixer");
        snd_mixer_close(handle);
        exit(1);
    }
    elem = snd_mixer_find_selem(handle,sid);
    if(!elem)
    {
        printf("Error finding simple control");
        snd_mixer_close(handle);
        exit(1);
    }

    print_vol_bar(elem,lcd);

    while(1)
    {
        if(oldvalue != encoder->value)
        {
            long change = encoder->value - oldvalue;
            int chn = 0;
            for(chn; chn <= SND_MIXER_SCHN_LAST; chn++)
            {
                long orig;
                snd_mixer_selem_get_playback_dB(elem,chn,&orig);
                snd_mixer_selem_set_playback_dB(elem,chn,orig + (change * 6),0);
            }
            print_vol_bar(elem,lcd);
            oldvalue = encoder->value;
        }
        delay(10);
    }

}
