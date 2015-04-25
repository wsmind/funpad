#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

#include "launchpad.h"
#include "partition.h"
#include "wav.h"

#define BUFFER_SIZE 256
#define POLYPHONY 16

#define BPM 79.0f
#define BAR_DURATION ((unsigned int)(60.0f * 44100.0f / BPM))
#define DELAY_LEFT_DURATION (BAR_DURATION / 2)
#define DELAY_RIGHT_DURATION (BAR_DURATION  * 2 / 3)

unsigned int min(unsigned int a, unsigned int b)
{
	return (a <= b) ? a : b;
}

// playing voices
typedef struct
{
	short *play_position;
	unsigned int remaining_frames;
	int aux_send;
} channel_t;

channel_t channels[POLYPHONY];

int main()
{
	memset(&channels, 0, sizeof(channels));
	
	sfx_t sfx[] = {
		load_sfx("data/Hhrruuuuu!!!.wav"),
		load_sfx("data/Kick.wav"),
		load_sfx("data/Bounce.wav"),
		load_sfx("data/OJ_Damn.wav"),
		load_sfx("data/Gucci_Mane_Yeah_(1).wav"),
		load_sfx("data/Gucci_Mane_Yeah_(2).wav")
	};
	
	sfx_t main_track = load_sfx("data/track.wav");
	channels[0].play_position = main_track.buffer;
	channels[0].remaining_frames = main_track.frame_count;
	
	partition_t *partition = load_partition("data/test.json");
	
	launchpad_open();
	launchpad_reset();
	launchpad_set_brightness(0xf);
	
	/*int x, y;
	for (y = 0; y < 8; y++)
	{
		for (x = 0; x < 8; x++)
		{
			launchpad_set_color(x, y, x % 4, y % 4);
		}
	}*/
	
	snd_pcm_t *pcm = 0;
	int err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0)
	{
		printf("Failed to open pcm device: %s\n", snd_strerror(err));
		return 1;
	}
	
	unsigned int rate = 44100;
	snd_pcm_hw_params_t *hw_params = 0;
	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_hw_params_any(pcm, hw_params);
	snd_pcm_hw_params_set_rate_resample(pcm, hw_params, 0);
	snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_rate_near(pcm, hw_params, &rate, 0);
	snd_pcm_hw_params_set_channels(pcm, hw_params, 2);
	snd_pcm_hw_params_set_period_size(pcm, hw_params, BUFFER_SIZE, 0);
	snd_pcm_hw_params_set_buffer_size(pcm, hw_params, BUFFER_SIZE * 4);
	
	err = snd_pcm_hw_params(pcm, hw_params);
	if (err < 0)
	{
		printf("Failed to apply pcm hardware settings: %s\n", snd_strerror(err));
		return 1;
	}
	
	snd_pcm_sw_params_t *sw_params = 0;
	snd_pcm_sw_params_alloca(&sw_params);
	snd_pcm_sw_params_current(pcm, sw_params);
	snd_pcm_sw_params_set_avail_min(pcm, sw_params, BUFFER_SIZE * 4);
	err = snd_pcm_sw_params(pcm, sw_params);
	if (err < 0)
	{
		printf("Failed to apply pcm software settings: %s\n", snd_strerror(err));
		return 1;
	}
	
	err = snd_pcm_prepare(pcm);
	if (err < 0)
	{
		printf("Failed to prepare pcm interface: %s\n", snd_strerror(err));
		return 1;
	}
	
	unsigned int i, j, delay_left_index = 0, delay_right_index = 0;
	short pcm_out[BUFFER_SIZE * 2];
	short aux_bus[BUFFER_SIZE * 2];
	short delay_left[DELAY_LEFT_DURATION];
	short delay_right[DELAY_RIGHT_DURATION];
	
	memset(delay_left, 0, sizeof(delay_left));
	memset(delay_right, 0, sizeof(delay_right));
	
	while (1)
	{
		char note[3];
		int status = launchpad_read(note);
		if (status > 0)
		{
			if (note[2] != 0)
			{
				note[2] = 0x4f;
				
				for (i = 0; i < POLYPHONY; i++)
				{
					if (!channels[i].remaining_frames)
					{
						unsigned int sfx_id = note[1] % 6;
						channels[i].play_position = sfx[sfx_id].buffer;
						channels[i].remaining_frames = sfx[sfx_id].frame_count;
						channels[i].aux_send = 1;
						break;
					}
				}
			}
			
			launchpad_write(note);
			
			printf("received: %02x\n", note[1]);
		}
		
		unsigned int track_time = main_track.frame_count - channels[0].remaining_frames - BUFFER_SIZE * 4;
		const float bpm = 79.0f;
		float track_beat = (float)track_time * bpm / 60.0f / 44100.0f;
		
/*		note[0] = 0x90;
		note[1] = 0x00; // row 0, column 0
		note[2] = (fmod(track_beat, 2.0f) >= 1.0f) ? 0x7f : 0x00;*/
		//launchpad_write(note);
//		launchpad_set_color(0, 0, 0, (fmod(track_beat, 2.0f) >= 1.0f) * 3);
//		launchpad_set_brightness((int)((sin(track_beat) * 0.5 + 0.5) * 15));

		note_t *pnote;
		for (i = 0, pnote = partition->notes; i < partition->note_count; i++, pnote++)
		{
			float note_time = track_beat - pnote->time;
			if ((note_time >= 0.0f) && (note_time < 1.0f))
				launchpad_set_color(pnote->x, pnote->y, 3, 3);
			else if ((note_time >= 1.0f) && (note_time < 2.0f))
				launchpad_set_color(pnote->x, pnote->y, 0, 0);
		}
		
		memset(pcm_out, 0, sizeof(pcm_out));
		memset(aux_bus, 0, sizeof(aux_bus));
		for (i = 0; i < POLYPHONY; i++)
		{
			unsigned int frames_to_mix = min(channels[i].remaining_frames, BUFFER_SIZE);
			short *channel_data = channels[i].play_position;
			for (j = 0; j < frames_to_mix * 2; j++)
				pcm_out[j] += *channel_data++ / 2;
			
			if (channels[i].aux_send)
			{
				channel_data = channels[i].play_position;
				for (j = 0; j < frames_to_mix * 2; j++)
					aux_bus[j] += *channel_data++ / 2;
			}
			
			channels[i].play_position = channel_data;
			channels[i].remaining_frames -= frames_to_mix;
		}
		
		for (i = 0; i < BUFFER_SIZE; i++)
		{
			aux_bus[i * 2 + 0] += delay_left[delay_left_index] * 2 / 3;
			delay_left[delay_left_index] = aux_bus[i * 2 + 0];
			delay_left_index = (delay_left_index + 1) % DELAY_LEFT_DURATION;
			
			aux_bus[i * 2 + 1] += delay_right[delay_right_index] * 2 / 3;
			delay_right[delay_right_index] = aux_bus[i * 2 + 1];
			delay_right_index = (delay_right_index + 1) % DELAY_RIGHT_DURATION;
		}
		
		for (i = 0; i < BUFFER_SIZE * 2; i++)
			pcm_out[i] += aux_bus[i] >> 1;
		
		snd_pcm_sframes_t frames = snd_pcm_writei(pcm, pcm_out, BUFFER_SIZE);
		if (frames < 0)
			snd_pcm_recover(pcm, frames, 0);
	}
	
	return 0;
}

