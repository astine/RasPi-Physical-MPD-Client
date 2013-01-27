#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <alsa/asoundlib.h>

#include "rotaryencoder/rotaryencoder.h"

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
                snd_mixer_selem_set_playback_dB(elem,chn,orig + (change * 2),0);
            }
            oldvalue = encoder->value;
        }
        delay(10);
    }

}
