#include <stdio.h>
#include <math.h>
#include <alsa/asoundlib.h>

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

typedef struct
{
	char magic[4];
	unsigned int file_length;
	char riff_type[4];
} riff_header_t;

typedef struct
{
	char chunk_name[4];
	unsigned int chunk_size;
} riff_chunk_header_t;

typedef struct
{
	unsigned short format_tag;
	unsigned short channels;
	unsigned int samples_per_second;
	unsigned int average_bytes_per_second;
	unsigned short block_align;
	unsigned short bits_per_sample;
} fmt_chunk_t;

typedef struct
{
	short *buffer; // always 16-bits LE, 2 channels
	unsigned int frame_count;
} sfx_t;

sfx_t load_sfx(const char *filename)
{
	printf("loading %s\n", filename);
	
	FILE *f = fopen(filename, "rb");
	assert(f);
	
	riff_header_t header;
	fread(&header, sizeof(header), 1, f);
	assert(!strncmp(header.magic, "RIFF", 4));
	assert(!strncmp(header.riff_type, "WAVE", 4));
	
	fmt_chunk_t format;
	short *output_buffer = 0;
	unsigned int data_size = 0;
	
	while (!output_buffer)
	{
		riff_chunk_header_t chunk_header;
		fread(&chunk_header, sizeof(chunk_header), 1, f);
		
		if (!strncmp(chunk_header.chunk_name, "fmt ", 4))
		{
			fread(&format, sizeof(format), 1, f);
			assert(format.channels == 2);
			assert(format.samples_per_second == 44100);
			assert(format.bits_per_sample == 16);
			
			fseek(f, chunk_header.chunk_size - sizeof(format), SEEK_CUR);
		}
		else if (!strncmp(chunk_header.chunk_name, "data", 4))
		{
			output_buffer = malloc(chunk_header.chunk_size);
			data_size = chunk_header.chunk_size;
			fread(output_buffer, chunk_header.chunk_size, 1, f);
			
			/*int i;
			for (i = 0; i < chunk_header.chunk_size / 4; i++)
				output_buffer[i] = output_buffer[i * 2];*/
		}
		else
		{
			fseek(f, chunk_header.chunk_size, SEEK_CUR);
		}
	}
	
	fclose(f);
	
	sfx_t sfx;
	sfx.buffer = output_buffer;
	sfx.frame_count = data_size / format.channels / (format.bits_per_sample >> 3);
	return sfx;
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
	//channels[0].play_position = main_track.buffer;
	//channels[0].remaining_frames = main_track.frame_count;
	
	snd_rawmidi_t *in = 0;
	snd_rawmidi_t *out = 0;
	int err = snd_rawmidi_open(&in, &out, "hw:1,0,0", SND_RAWMIDI_NONBLOCK);
	if (err < 0)
	{
		printf("Failed to open midi device: %s\n", snd_strerror(err));
		return 1;
	}
	
	snd_pcm_t *pcm = 0;
	err = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
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
		int status = snd_rawmidi_read(in, &note, 3);
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
			
			snd_rawmidi_write(out, &note, 3);
			
			printf("received: %02x\n", note[1]);
		}
		
		unsigned int track_time = main_track.frame_count - channels[0].remaining_frames - BUFFER_SIZE * 4;
		const float bpm = 79.0f;
		float track_beat = (float)track_time * bpm / 60.0f / 44100.0f;
		
		note[0] = 0x90;
		note[1] = 0x00; // row 0, column 0
		note[2] = (fmod(track_beat, 2.0f) >= 1.0f) ? 0x7f : 0x00;
		snd_rawmidi_write(out, &note, 3);
		
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

