#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "launchpad.h"
#include "mixer.h"
#include "partition.h"
#include "wav.h"

#define PRE_TIME 2.0f
#define POST_TIME 1.0f

static float time = 0.0f;

typedef enum
{
	STATE_MENU,
	STATE_GAME
} gamestate_t;

static gamestate_t current_state;

static void menu_enter();
static void menu_leave();
static void menu_update();
static void game_enter(const char *filename);
static void game_leave();
static void game_update();


static sfx_t menu_track;
static unsigned int menu_channel;

static void menu_enter()
{
	launchpad_reset();
	
	menu_track = load_sfx("data/wav/harmonica.wav");
	menu_channel = mixer_play(menu_track, 0, 1, 1);
}

static void menu_leave()
{
	mixer_stop(menu_channel);
	unload_sfx(menu_track);
}

static void menu_update()
{
	int i;
	for (i = 0; i < 8; i++)
	{
		float fcolor = fmod((float)i / 8.0f + time * 2.0f, 1.0f);
		int color = (int)(fcolor * 2.0f + 1.0f);
		launchpad_set_color(i, 0, color, 0);
		launchpad_set_color(7, i, color, color);
		launchpad_set_color(7 - i, 7, 0, color);
		launchpad_set_color(0, 7 - i, color, color);
	}
	
	launchpad_set_color(2, 3, 0, 3);
	launchpad_set_color(3, 3, 0, 3);
	launchpad_set_color(2, 4, 0, 3);
	launchpad_set_color(3, 4, 0, 3);
	
	launchpad_set_color(4, 3, 3, 3);
	launchpad_set_color(5, 3, 3, 3);
	launchpad_set_color(4, 4, 3, 3);
	launchpad_set_color(5, 4, 3, 3);
	
	int x, y;
	if (launchpad_get_input(0, &x, &y))
	{
		if ((y >= 3) && (y <= 4))
		{
			if ((x >= 2) && (x <= 3))
			{
				menu_leave();
				current_state = STATE_GAME;
				game_enter("data/json/danse.json");
			}
			if ((x >= 4) && (x <= 5))
			{
				menu_leave();
				current_state = STATE_GAME;
				game_enter("data/json/folk.json");
			}
		}
	}
}

static partition_t *partition;
static float note_scores[MAX_NOTES];
static sfx_t main_track;
static int main_channel;

static void game_enter(const char *filename)
{
	partition = load_partition(filename);
	
	main_track = load_sfx(partition->track);
	int skip_frames = 44100 * 2 * partition->skip_bars;
	main_channel = mixer_play(main_track, skip_frames, 0, 0);
	
	memset(note_scores, 0, sizeof(note_scores));
	
	launchpad_reset();
}

static void game_leave()
{
	unload_partition(partition);
	unload_sfx(main_track);
}

static void game_update()
{
	int i;
	
	int down = 0, x = -1, y = -1;
	if (launchpad_get_input(&down, &x, &y))
	{
		if (down)
		{
			launchpad_set_color(x, y, 0, 3);
		}
		else
		{
			launchpad_set_color(x, y, 0, 0);
		}
		
		printf("received: %d %d\n", x, y);
	}
	
	unsigned int track_time = main_track.frame_count - mixer_get_remaining_frames(main_channel) - BUFFER_SIZE * 4;
	float track_beat = (float)track_time * partition->bpm / 60.0f / 44100.0f;
	
	note_t *note;
	for (i = 0, note = partition->notes; i < partition->note_count; i++, note++)
	{
		float note_time = track_beat - note->time;
		
		if ((note->x == x) && (note->y == y) && (note_scores[i] == 0.0f))
		{
			if ((note_time > -0.5f) && (note_time < 0.5f))
			{
				note_scores[i] = 1.0f - fabs(note_time) * 2.0f;
				printf("note %d scored %f\n", i, note_scores[i]);
			}
		}
		
		if ((note_time >= -PRE_TIME) && (note_time < 0.0f))
		{
			if (note_time < -PRE_TIME * 0.6f)
				launchpad_set_color(note->x, note->y, 1, 1);
			else if (note_time < -PRE_TIME * 0.3f)
				launchpad_set_color(note->x, note->y, 2, 2);
			else
				launchpad_set_color(note->x, note->y, 3, 3);
		}
		else if ((note_time >= 0.0f) && (note_time < POST_TIME))
		{
			if (note_scores[i] > 0.0f)
			{
				launchpad_set_color(note->x, note->y, 0, 3);
			}
			else
			{
				if (note_time < POST_TIME * 0.5f)
					launchpad_set_color(note->x, note->y, 1, 3);
				else
					launchpad_set_color(note->x, note->y, 1, 0);
			}
		}
		else if ((note_time >= POST_TIME) && (note_time < POST_TIME + 0.2f))
		{
			launchpad_set_color(note->x, note->y, 0, 0);
		}
	}
}

int main(int argc, char** argv)
{
	/*sfx_t sfx[] = {
		load_sfx("data/wav/Hhrruuuuu!!!.wav"),
		load_sfx("data/wav/Kick.wav"),
		load_sfx("data/wav/Bounce.wav"),
		load_sfx("data/wav/OJ_Damn.wav"),
		load_sfx("data/wav/Gucci_Mane_Yeah_(1).wav"),
		load_sfx("data/wav/Gucci_Mane_Yeah_(2).wav")
	};*/
	
	mixer_open();
	
	launchpad_open();
	launchpad_reset();
	launchpad_set_brightness(0xf);
	
	//game_enter(argv[1]);
	current_state = STATE_MENU;
	menu_enter();
	
	while (1)
	{
		switch (current_state)
		{
			case STATE_MENU: menu_update(); break;
			case STATE_GAME: game_update(); break;
		}
		
		mixer_render();
		time += (float)BUFFER_SIZE / 44100.0f;
	}
	
	return 0;
}

