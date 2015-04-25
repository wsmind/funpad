#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "wav.h"

sfx_t load_sfx(const char *filename)
{
	printf("loading wav %s\n", filename);
	
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

