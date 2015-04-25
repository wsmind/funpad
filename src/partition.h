#ifndef PARTITION_H
#define PARTITION_H

typedef struct
{
	float time;
	unsigned int x;
	unsigned int y;
} note_t;

typedef struct
{
	unsigned int note_count;
	note_t *notes;
} partition_t;

partition_t *load_partition(const char *filename);

#endif // PARTITION_H

