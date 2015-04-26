#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

#include "mixer.h"

#define BAR_DURATION (60 * 44100 / 120)
#define DELAY_LEFT_DURATION (BAR_DURATION / 2)
#define DELAY_RIGHT_DURATION (BAR_DURATION  * 2 / 3)

// playing voices
typedef struct
{
	short *play_position;
	unsigned int remaining_frames;
	int aux_send;
} channel_t;

static channel_t channels[POLYPHONY];

unsigned int delay_left_index = 0, delay_right_index = 0;
static short pcm_out[BUFFER_SIZE * 2];
static short aux_bus[BUFFER_SIZE * 2];
static short delay_left[DELAY_LEFT_DURATION];
static short delay_right[DELAY_RIGHT_DURATION];

static snd_pcm_t *pcm = 0;

void mixer_open()
{
	memset(&channels, 0, sizeof(channels));
	int err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0)
	{
		printf("Failed to open pcm device: %s\n", snd_strerror(err));
		exit(1);
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
		exit(1);
	}
	
	snd_pcm_sw_params_t *sw_params = 0;
	snd_pcm_sw_params_alloca(&sw_params);
	snd_pcm_sw_params_current(pcm, sw_params);
	snd_pcm_sw_params_set_avail_min(pcm, sw_params, BUFFER_SIZE * 4);
	err = snd_pcm_sw_params(pcm, sw_params);
	if (err < 0)
	{
		printf("Failed to apply pcm software settings: %s\n", snd_strerror(err));
		exit(1);
	}
	
	err = snd_pcm_prepare(pcm);
	if (err < 0)
	{
		printf("Failed to prepare pcm interface: %s\n", snd_strerror(err));
		exit(1);
	}
	
	memset(delay_left, 0, sizeof(delay_left));
	memset(delay_right, 0, sizeof(delay_right));
}

unsigned int mixer_play(sfx_t sfx, unsigned int skip_frames, unsigned int aux_send)
{
	int i;
	for (i = 0; i < POLYPHONY; i++)
	{
		if (!channels[i].remaining_frames)
		{
			channels[i].play_position = sfx.buffer;
			channels[i].remaining_frames = sfx.frame_count;
			channels[i].aux_send = aux_send;
			return i;
		}
	}
	
	assert(0); // no free channel
}

unsigned int mixer_get_remaining_frames(unsigned int channel)
{
	return channels[channel].remaining_frames;
}

void mixer_render()
{
	int i, j;
	
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

unsigned int min(unsigned int a, unsigned int b)
{
	return (a <= b) ? a : b;
}

