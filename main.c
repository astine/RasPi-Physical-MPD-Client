#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>
#include <mpd/client.h>

#include "rotaryencoder/rotaryencoder.h"
#include "lcd/lcd.h"

struct encoder encoder;

struct mpd_connection *connection = NULL;

static char card[64] = "default";
snd_mixer_t *handle= NULL;

void sig_handler(int signo)
{
    if(signo == SIGINT)
    {
        printf("SIGINT recieved\n");
        if(handle != NULL)
            snd_mixer_close(handle);
        if(connection != NULL)
            mpd_connection_free(connection);
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
    for(;idx<volp;idx++)
        volbar[idx] = '=';
    for(;idx<LCD_WIDTH;idx++)
        volbar[idx] = '_';
    volbar[LCD_WIDTH] = '\0';

    lcd_string(lcd,volbar,2);
}

void printErrorAndExit(struct mpd_connection *conn)
{
    const char *message;

    assert(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS);

    message = mpd_connection_get_error_message(conn);

    fprintf(stderr, "error: %s\n", message);
    mpd_connection_free(conn);
    exit(EXIT_FAILURE);
}

void print_song_title(struct lcd lcd)
{
    //Find the current song
    struct mpd_song *song = mpd_run_current_song(connection);

    //If there is no current song, find first in playlist
    if(song == NULL)
        song = mpd_run_get_queue_song_pos(connection, 0);

    const char *title;
    if(song != NULL)
        title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    else 
        //If song is NULL, the playlist is probably empty
        title = "Empty playlist";

    lcd_marquee(lcd,title,1);

    if(song != NULL)
        mpd_song_free(song);
}

static struct mpd_connection *setup_connection(void)
{
    struct mpd_connection *conn;

    conn = mpd_connection_new("10.0.0.2", 6600, 0);
    if (conn == NULL) 
    {
        fputs("Out of memory\n", stderr);
        exit(EXIT_FAILURE);
    }

    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS)
        printErrorAndExit(conn);

    return conn;
}

int main()
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

    connection = setup_connection();

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
            for(; chn <= SND_MIXER_SCHN_LAST; chn++)
            {
                long orig;
                snd_mixer_selem_get_playback_dB(elem,chn,&orig);
                snd_mixer_selem_set_playback_dB(elem,chn,orig + (change * 6),0);
            }
            print_vol_bar(elem,lcd);
            oldvalue = encoder->value;
        }
        delay(10);
	print_song_title(lcd);
    }

    if(handle != NULL)
        snd_mixer_close(handle);
    if(connection != NULL)
        mpd_connection_free(connection);

    return 0;
}
