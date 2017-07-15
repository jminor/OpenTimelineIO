#include <stdlib.h>
#include <stdio.h>
#include "OpenTimelineIo.h"



void otio_generate_random(OTIOObject *parent, uint recursions, uint count, uint *seed)
{
	OTIOTime time;
	OTIOObjectType type;
	OTIOObject *object, *media;
	uint i;
	time.time_scale = 24;
	for(i = 0; i < count; i++)
	{
		if(recursions != 0)
			type = (*seed) % (ORIO_OT_MEDIA_REFFERENCE);
		else
			type = ORIO_OT_CLIP + (*seed) % (ORIO_OT_MEDIA_REFFERENCE - ORIO_OT_CLIP);
		(*seed)++;
		object = otio_object_create(type, "a thing");
		otio_object_insert_before(parent, object, NULL);
		if(type < ORIO_OT_COMPOSABLE_END)
		{
			time.time_value = (*seed) % 24;
			(*seed)++;
			otio_composable_start_time_set(object, time);
			time.time_value = (*seed) % 56;
			(*seed)++;
			otio_composable_duration_time_set(object, time);
		}
		if(type == ORIO_OT_TRACK || type == ORIO_OT_SEQUENCE)
			otio_generate_random(object, recursions - 1, count, seed);
		switch(type)
		{
			case ORIO_OT_TRACK :
			break;
			case ORIO_OT_SEQUENCE :
			break;
			case ORIO_OT_CLIP :
				media = otio_object_create(ORIO_OT_MEDIA_REFFERENCE, "some media");
				otio_object_insert_before(object, media, NULL);
			break;
			case ORIO_OT_FILLER :
			break;
			case ORIO_OT_TRANSITION :
				otio_transition_type_set(object, "SMPTE_Dissolve");
				time.time_value = (*seed) % 24;
				(*seed)++;
				otio_transition_in_time_set(object, time);
				time.time_value = (*seed) % 13;
				(*seed)++;
				otio_transition_out_time_set(object, time);
			break;
			case ORIO_OT_EFFECT :
			break;
			case ORIO_OT_MARKER :
				time.time_value = (*seed) % 32;
				(*seed)++;
				otio_marker_time_set(object, time);
				otio_marker_message_set(object, "Awsome stuff!");
			break;
			case ORIO_OT_MEDIA_REFFERENCE :
				otio_media_reference_kind_set(object, "main");
				{
					char file[256];
					sprintf(file, "./scene_%s.mov", *seed);
					otio_media_reference_uri_set(object, file);
				}
			break;
		}
	}
}


char *assemble_file_load(char *file)
{
	char *buffer;
	uint size;
	FILE *f;
	f = fopen(file, "r");
	if(f == NULL)
		return NULL;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);
	buffer = malloc(size + 1);
	size = fread(buffer, 1, size, f);
	fclose(f);
	buffer[size] = 0;
	return buffer;
}

int main(int argc, char **argv)
{	
	OTIOObject *object;
	uint seed = 0;
	object = otio_object_create(ORIO_OT_SEQUENCE, "My sequence");
	otio_generate_random(object, 2, 4, &seed);
	otio_save("random_output.json", object);
	printf("Done");
	object = otio_load(assemble_file_load("random_output.json"));
	otio_save("random_re_output.json", object);
	return 1;	
}