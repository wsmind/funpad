#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "cJSON.h"
#include "partition.h"

partition_t *load_partition(const char *filename)
{
	printf("loading partition %s\n", filename);
	
	FILE *f = fopen(filename, "r");
	assert(f);
	
	fseek(f, 0, SEEK_END);
	unsigned int size = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char *contents = malloc(size);
	fread(contents, size, 1, f);
	
	fclose(f);
	
	cJSON *root = cJSON_Parse(contents);
	assert(root);
	
	partition_t *partition = malloc(sizeof(partition_t));
	
	cJSON *notesJson = cJSON_GetObjectItem(root, "notes");
	assert(notesJson);
	
	partition->note_count = cJSON_GetArraySize(notesJson);
	printf("%d notes!\n", partition->note_count);
	
	partition->notes = malloc(sizeof(note_t) * partition->note_count);
	cJSON *noteJson = notesJson->child;
	int i;
	for (i = 0; i < partition->note_count; i++)
	{
		partition->notes[i].time = (float)cJSON_GetArrayItem(noteJson, 0)->valuedouble;
		partition->notes[i].x = (float)cJSON_GetArrayItem(noteJson, 1)->valueint;
		partition->notes[i].y = (float)cJSON_GetArrayItem(noteJson, 2)->valueint;
		noteJson = noteJson->next;
	}
	
	cJSON_Delete(root);
	
	free(contents);
	
	return partition;
}

