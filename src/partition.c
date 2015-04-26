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
	assert(cJSON_GetObjectItem(root, "patterns"));
	assert(cJSON_GetObjectItem(root, "sheet"));
	
	partition->track = strdup(cJSON_GetObjectItem(root, "track")->valuestring);
	partition->bpm = (float)cJSON_GetObjectItem(root, "bpm")->valuedouble;
	
	if (cJSON_GetObjectItem(root, "skip_bars"))
		partition->skip_bars = cJSON_GetObjectItem(root, "skip_bars")->valueint;
	else
		partition->skip_bars = 0;
	
	cJSON *patternsJson = cJSON_GetObjectItem(root, "patterns");
	cJSON *sheetJson = cJSON_GetObjectItem(root, "sheet");
	
	partition->note_count = 0;
	partition->length = 0;
	cJSON *currentPattern = sheetJson->child;
	while (currentPattern)
	{
		char *patternName = currentPattern->valuestring;
		cJSON *patternJson = cJSON_GetObjectItem(patternsJson, patternName);
		assert(cJSON_GetObjectItem(patternJson, "length"));
		assert(cJSON_GetObjectItem(patternJson, "notes"));
		
		int pattern_length = cJSON_GetObjectItem(patternJson, "length")->valueint;
		cJSON *noteJson = cJSON_GetObjectItem(patternJson, "notes")->child;
		while (noteJson)
		{
			partition->notes[partition->note_count].time = (float)cJSON_GetArrayItem(noteJson, 0)->valuedouble + (float)partition->length;
			partition->notes[partition->note_count].x = (float)cJSON_GetArrayItem(noteJson, 1)->valueint;
			partition->notes[partition->note_count].y = (float)cJSON_GetArrayItem(noteJson, 2)->valueint;
			//printf("note %f (%d, %d)\n", partition->notes[partition->note_count].time, partition->notes[partition->note_count].x, partition->notes[partition->note_count].y);
			partition->note_count++;
			noteJson = noteJson->next;
		}
		
		partition->length += pattern_length;
		currentPattern = currentPattern->next;
	}
	
	cJSON_Delete(root);
	
	free(contents);
	
	printf("%d notes! (%d beats)\n", partition->note_count, partition->length);
	
	return partition;
}

void unload_partition(partition_t *partition)
{
	free(partition);
}

