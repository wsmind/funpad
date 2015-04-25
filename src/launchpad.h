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

#endif // LAUNCHPAD_H

