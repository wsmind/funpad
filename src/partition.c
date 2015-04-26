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
	
	assert(cJSON_GetObjectItem(root, "track"));
	assert(cJSON_GetObjectItem(root, "bpm"));
	assert(cJSON_GetObjectItem(root, "notes"));
	
	partition->track = strdup(cJSON_GetObjectItem(root, "track")->valuestring);
	partition->bpm = (float)cJSON_GetObjectItem(root, "bpm")->valuedouble;
	
	if (cJSON_GetObjectItem(root, "skip_bars"))
		partition->skip_bars = cJSON_GetObjectItem(root, "skip_bars")->valueint;
	else
		partition->skip_bars = 0;
	
	cJSON *notesJson = cJSON_GetObjectItem(root, "notes");
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

