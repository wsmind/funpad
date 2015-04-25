#include <stdio.h>
#include <alsa/asoundlib.h>

#include "launchpad.h"

static snd_rawmidi_t *in = 0;
static snd_rawmidi_t *out = 0;

void launchpad_open()
{
	int err = snd_rawmidi_open(&in, &out, "hw:2,0,0", SND_RAWMIDI_NONBLOCK);
	if (err < 0)
	{
		printf("Failed to open midi device: %s\n", snd_strerror(err));
		exit(1);
	}
}

int launchpad_read(char note[3])
{
	return snd_rawmidi_read(in, note, 3);
}

void launchpad_write(char note[3])
{
	snd_rawmidi_write(out, note, 3);
}

void launchpad_reset()
{
	char command[3] = {0xb0, 0x0, 0x0};
	launchpad_write(command);
}

void launchpad_set_brightness(unsigned int brightness)
{
	assert(brightness < 0x10);
	
	unsigned int denominator_plus_3 = 0xf - brightness;
	
	char command[3] = {0xb0, 0x1e, denominator_plus_3};
	launchpad_write(command);
}

void launchpad_set_color(unsigned int x, unsigned int y, unsigned int red, unsigned int green)
{
	assert(x < 8);
	assert(y < 8);
	assert(red < 4);
	assert(green < 4);
	
	char note[3];
	note[0] = 0x90; // note on
	note[1] = (y << 4) | x;
	note[2] = (green << 4) | 0x0c | red;
	launchpad_write(note);
}

