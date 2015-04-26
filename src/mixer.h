#ifndef MIXER_H
#define MIXER_H

#include "wav.h"

#define BUFFER_SIZE 256
#define POLYPHONY 16

void mixer_open();
unsigned int mixer_play(sfx_t sfx, unsigned int skip_frames, unsigned int aux_send);
unsigned int mixer_get_remaining_frames(unsigned int channel);

void mixer_render();

unsigned int min(unsigned int a, unsigned int b);

#endif // MIXER_H

