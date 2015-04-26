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

float *note_scores;

int main(int argc, char** argv)
{
	int i;
	
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
	
	partition_t *partition = load_partition(argv[1]);
	
	sfx_t main_track = load_sfx(partition->track);
	int skip_frames = 44100 * 2 * partition->skip_bars;
	int main_channel = mixer_play(main_track, skip_frames, 0);
	
	note_scores = malloc(sizeof(float) * partition->note_count);
	memset(note_scores, 0, sizeof(float) * partition->note_count);
	
	/*int x, y;
	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			launchpad_set_color(x, y, x % 4, y % 4);
		}
	}*/
	
	
	while (1)
	{
		int down = 0, x = -1, y = -1;
		/*char note[3];
		int status = launchpad_read(note);
		if (status > 0)
		{
			x = min(launchpad_get_x(note), 7);
			y = min(launchpad_get_y(note), 7);
			
			if (note[2] != 0)
			{
				launchpad_set_color(x, y, 0, 3);
			}
			else
			{
				launchpad_set_color(x, y, 0, 0);
			}
			
			//launchpad_write(note);
			
			printf("received: %02x\n", note[1]);
		}*/
		
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
		
		note_t *pnote;
		for (i = 0, pnote = partition->notes; i < partition->note_count; i++, pnote++)
		{
			float note_time = track_beat - pnote->time;
			
			if ((pnote->x == x) && (pnote->y == y) && (note_scores[i] == 0.0f))
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
					launchpad_set_color(pnote->x, pnote->y, 1, 1);
				else if (note_time < -PRE_TIME * 0.3f)
					launchpad_set_color(pnote->x, pnote->y, 2, 2);
				else
					launchpad_set_color(pnote->x, pnote->y, 3, 3);
			}
			else if ((note_time >= 0.0f) && (note_time < POST_TIME))
			{
				if (note_scores[i] > 0.0f)
				{
					launchpad_set_color(pnote->x, pnote->y, 0, 3);
				}
				else
				{
					if (note_time < POST_TIME * 0.5f)
						launchpad_set_color(pnote->x, pnote->y, 1, 3);
					else
						launchpad_set_color(pnote->x, pnote->y, 1, 0);
				}
			}
			else if ((note_time >= POST_TIME) && (note_time < POST_TIME + 0.2f))
			{
				launchpad_set_color(pnote->x, pnote->y, 0, 0);
			}
		}
		
		mixer_render();
	}
	
	return 0;
}

