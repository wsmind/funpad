#ifndef LAUNCHPAD_H
#define LAUNCHPAD_H

void launchpad_open();

int launchpad_read(char note[3]);
void launchpad_write(char note[3]);

void launchpad_reset();

// valid brightness goes from 0x0 to 0xf
void launchpad_set_brightness(unsigned int brightness);

// x and y should be in 0-7
// red and green are 0-3
void launchpad_set_color(unsigned int x, unsigned int y, unsigned int red, unsigned int green);

// pressed is a boolean (0 = key up, 1 = key down)
// returns non-zero if there was an input, 0 otherwise
int launchpad_get_input(int *down, int *x, int *y);

unsigned int launchpad_is_down(char note[3]);
unsigned int launchpad_get_x(char note[3]);
unsigned int launchpad_get_y(char note[3]);

#endif // LAUNCHPAD_H

