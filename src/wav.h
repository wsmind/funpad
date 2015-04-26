#ifndef WAV_H
#define WAV_H

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

sfx_t load_sfx(const char *filename);
void unload_sfx(sfx_t sfx);

#endif // WAV_H

