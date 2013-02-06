#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <time.h>
#include <wiringPi.h>
#include <alsa/asoundlib.h>
#include <mpd/client.h>

#include "rotaryencoder/rotaryencoder.h"
#include "button/button.h"
#include "lcd/lcd.h"

#define MPD_LOCK 0

struct encoder encoder;

struct mpd_connection *connection = NULL;

static char card[64] = "default";
snd_mixer_t *handle = NULL;
snd_mixer_elem_t *elem = NULL;

int current_song = 0;

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

int play_queue_length()
{
    struct mpd_song *song;
    
    if (!mpd_send_list_queue_meta(connection))
	printErrorAndExit(connection);
    
    int count = 0;
    
    while ((song = mpd_recv_song(connection)) != NULL) {
	mpd_song_free(song);
	count++;
    }
    
    if (!mpd_response_finish(connection))
	printErrorAndExit(connection);
    
    return count;
}

void print_song_title(struct lcd lcd)
{
    piLock(MPD_LOCK);
    //Find the current song
    struct mpd_status *status = mpd_run_status(connection);
    if (status == NULL)
	printErrorAndExit(connection);

    struct mpd_song *song = mpd_run_current_song(connection);
    
    //If there is no current song, find first in playlist
    if(song == NULL || mpd_status_get_state(status) == MPD_STATE_STOP)
        song = mpd_run_get_queue_song_pos(connection, current_song);

    const char *title;
    if(song != NULL)
        title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    else 
        //If song is NULL, the playlist is probably empty
        title = "Empty playlist";

    piUnlock(MPD_LOCK);

    lcd_string(lcd,title,1);

    if(song != NULL)
        mpd_song_free(song);
    if(status != NULL)
	mpd_status_free(status);
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

void skip_song(int direction)
{
    piLock(MPD_LOCK);
    struct mpd_status *status = mpd_run_status(connection);
    if (status == NULL)
	printErrorAndExit(connection);
    
    if(mpd_status_get_state(status) == MPD_STATE_STOP)
	{
	    if (direction == 1)
		{
		    if (current_song < play_queue_length())
			{
			    current_song++;
			}
		}
	    else
		{
		    if (current_song > 0)
			current_song--;
		}
	}
    else
	{
	    if (direction == 1)
		mpd_run_next(connection);
	    else
		mpd_run_previous(connection);
	}
    mpd_status_free(status);
    piUnlock(MPD_LOCK);
}

void play_pause_callback(int state)
{
    static long otime = 0;

    if(state == 1)
	{
	if(time(NULL) <= otime + 1)
	    {
		piLock(MPD_LOCK);
		struct mpd_status *status = mpd_run_status(connection);
		if (status == NULL)
		    printErrorAndExit(connection);

		if(mpd_status_get_state(status) == MPD_STATE_STOP)
		    {
			printf("Starting Player\n");
			mpd_run_play_pos(connection, current_song);
		    }
		else
		    {
			printf("Toggling Pause\n");
			mpd_run_toggle_pause(connection);
		    }
		mpd_status_free(status);
		piUnlock(MPD_LOCK);
	    }
	}
    else
	otime = time(NULL);
}

void mute_unmute_callback(int state)
{
    if(state == 1)
	{
	    int ival;
            printf("Toggling Mute\n");
	    snd_mixer_selem_get_playback_switch(elem, 0, &ival);
	    snd_mixer_selem_set_playback_switch(elem, 0, !ival);
	}
}

int main()
{
    printf("Starting...\n");
    
    if(signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\n Can't catch SIGINT. \n");

    wiringPiSetup();
    piHiPri(99);
    struct encoder *vol_selector = setupencoder(12,13);
    if(vol_selector == NULL) { exit(1); }
    int oldvalue = vol_selector->value;

    struct encoder *song_selector = setupencoder(15,16);
    if(song_selector == NULL) { exit(1); }
    int oldsongvalue = song_selector->value;

    struct lcd lcd = lcd_init(8, 9, 7, 0, 2, 3);
    struct button *button = setup_button(14, play_pause_callback);
    if(button == NULL) { exit(1); }
    setup_button(1, mute_unmute_callback);

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

    long button_timer = time(NULL);
    print_vol_bar(elem,lcd);

    while(1)
    {
        if(oldvalue != vol_selector->value)
        {
            int change = vol_selector->value - oldvalue;
            int chn = 0;
            for(; chn <= SND_MIXER_SCHN_LAST; chn++)
            {
		printf("Changing volume: %d\n", change);
                long orig;
                snd_mixer_selem_get_playback_dB(elem,chn,&orig);
                snd_mixer_selem_set_playback_dB(elem,chn,orig + (change * 6),0);
            }
            print_vol_bar(elem,lcd);
            oldvalue = vol_selector->value;
        }

	if(oldsongvalue > song_selector->value + 4)
	  {
	    printf("Next Song\n");
	    skip_song(1);
	    oldsongvalue = song_selector->value;
	  }
	else if(oldsongvalue < song_selector->value - 4)
	  {
	    printf("Previous Song\n");
	    skip_song(0);
	    oldsongvalue = song_selector->value;
	  }

	if (button->state == 1)
	    button_timer = time(NULL);
	
	if ((time(NULL) - button_timer) > 1)
	    {
		printf("Stopping Player\n");
		piLock(MPD_LOCK);
		mpd_run_stop(connection);
		piUnlock(MPD_LOCK);
		button_timer = time(NULL);
	    }

	print_song_title(lcd);
        delay(10);
    }

    if(sid != NULL)
	snd_mixer_selem_id_free(sid);
    if(handle != NULL)
        snd_mixer_close(handle);
    if(connection != NULL)
        mpd_connection_free(connection);

    return 0;
}
