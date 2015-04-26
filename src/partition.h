#ifndef PARTITION_H
#define PARTITION_H

#define MAX_NOTES 1024

typedef struct
{
	float time;
	unsigned int x;
	unsigned int y;
} note_t;

typedef struct
{
	char *track;
	float bpm;
	int skip_bars;
	unsigned int length;
	unsigned int note_count;
	note_t notes[MAX_NOTES];
} partition_t;

partition_t *load_partition(const char *filename);
void unload_partition(partition_t *partition);

#endif // PARTITION_H

