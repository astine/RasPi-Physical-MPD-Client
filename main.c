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
#include "led/led.h"

#define MPD_LOCK 0

#define exp10(x) (exp((x) * log(10)))

struct encoder encoder;

struct mpd_connection *connection = NULL;

static char card[64] = "hw:0";
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

//get the volume as a floating point number 0..1
double get_normalized_volume(snd_mixer_elem_t *elem)
{
    long max, min, value;
    int err;

    err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
    if(err < 0)
    {
        printf("Error getting volume\n");
	return 0;
    }
    err = snd_mixer_selem_get_playback_dB(elem,0,&value);
    if(err < 0)
    {
        printf("Error getting volume\n");
	return 0;
    }
    
    //Perceived 'loudness' does not scale linearly with the actual decible level
    //it scales logarithmically
    return exp10((value - max) / 6000.0);
}

//set the volume from a floating point number 0..1
void set_normalized_volume(snd_mixer_elem_t *elem, double volume)
{
    long min, max, value;
    int err;

    if(volume < 0.017170)
	volume = 0.017170;
    else if (volume > 1.0)
	volume = 1.0;

    err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
    if(err < 0)
    {
        printf("Error setting volume\n");
	return;
    }

    //Perceived 'loudness' does not scale linearly with the actual decible level
    //it scales logarithmically
    value = lrint(6000.0 * log10(volume)) + max;
    snd_mixer_selem_set_playback_dB(elem, 0, value, 0);
}

void print_vol_bar(snd_mixer_elem_t *elem,struct lcd lcd)
{
    int volbar_length = rint(get_normalized_volume(elem) * (double)LCD_WIDTH-1);
    char volbar[LCD_WIDTH];

    int idx = 0;
    volbar[idx++]='-';
    for(;idx<volbar_length;idx++)
        volbar[idx] = '=';
    for(;idx<LCD_WIDTH-1;idx++)
        volbar[idx] = '_';
    volbar[LCD_WIDTH-1] = '+';
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

    //Current song is either playing song, or one player remembers
    struct mpd_song *song;
    if(mpd_status_get_state(status) == MPD_STATE_STOP)
        song = mpd_run_get_queue_song_pos(connection, current_song);
    else
        song = mpd_run_current_song(connection);
    
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

    conn = mpd_connection_new("10.0.0.3", 6600, 0);
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

void set_play_indicator(struct led play_indicator)
{
    piLock(MPD_LOCK);
    struct mpd_status *status = mpd_run_status(connection);
    switch (mpd_status_get_state(status))
    {
    case MPD_STATE_PLAY:
        light_led_color(play_indicator, LED_COLOR_GREEN);
        break;
    case MPD_STATE_PAUSE:
        light_led_color(play_indicator, LED_COLOR_YELLOW);
        break;
    case MPD_STATE_STOP:
    default:
        light_led_color(play_indicator, LED_COLOR_RED);
    }
    mpd_status_free(status);
    piUnlock(MPD_LOCK);
}

void set_mute_indicator(struct led mute_indicator)
{
    int ival;
    snd_mixer_selem_get_playback_switch(elem, 0, &ival);
    if (ival == 1)
        light_led_color(mute_indicator, LED_COLOR_RED);
    else
        light_led_color(mute_indicator, LED_COLOR_GREEN);
}


int main()
{
    printf("Starting...\n");
    
    if(signal(SIGINT, sig_handler) == SIG_ERR)
        printf("\n Can't catch SIGINT. \n");

    wiringPiSetup();
    piHiPri(99);
    struct encoder *vol_selector = setupencoder(15,16);
    if(vol_selector == NULL) { exit(1); }
    int oldvalue = vol_selector->value;

    struct encoder *song_selector = setupencoder(13,12);
    if(song_selector == NULL) { exit(1); }
    int oldsongvalue = song_selector->value;

    struct lcd lcd = lcd_init(3, 2, 0, 7, 9, 8);
    struct button *button = setup_button(14, play_pause_callback);
    if(button == NULL) { exit(1); }
    setup_button(1, mute_unmute_callback);

    struct led play_indicator = led_init(6,11,10);
    struct led mute_indicator = led_init(5,4,-1);

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

    while(1)
    {
        if(oldvalue != vol_selector->value)
        {
            int change = vol_selector->value - oldvalue;
            int chn = 0;
            for(; chn <= SND_MIXER_SCHN_LAST; chn++)
            {
		double vol = get_normalized_volume(elem);
		printf("Changing volume: %f\n", vol + (change * 0.00065105));
		set_normalized_volume(elem, vol + (change * 0.00065105));
            }
            oldvalue = vol_selector->value;
        }

	if(oldsongvalue > song_selector->value + 2)
	  {
	    printf("Next Song\n");
	    skip_song(1);
	    oldsongvalue = song_selector->value;
	  }
	else if(oldsongvalue < song_selector->value - 2)
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
		//Stop player
		mpd_run_stop(connection);

		//Set "current song" to the song that was being played
        	struct mpd_song *song = mpd_run_current_song(connection);
		current_song = mpd_song_get_pos(song);
		mpd_song_free(song);

		piUnlock(MPD_LOCK);
		button_timer = time(NULL);
	    }

        print_vol_bar(elem,lcd);
	print_song_title(lcd);
        set_play_indicator(play_indicator);
        set_mute_indicator(mute_indicator);
        delay(25);
    }

    if(sid != NULL)
	snd_mixer_selem_id_free(sid);
    if(handle != NULL)
        snd_mixer_close(handle);
    if(connection != NULL)
        mpd_connection_free(connection);

    return 0;
}
