#include <stdlib.h>
#include <stdio.h>

#include "OpenTimelineIO.h"
#include "assemble_json.h"


char *otio_text_load(char *file_name)
{
	char *buffer;
	uint size;
	FILE *f;
	f = fopen(file_name, "r");
	if(f == NULL)
	{
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);
	buffer = malloc(size + 1);
	size = fread(buffer, 1, size, f);
	fclose(f);
	buffer[size] = 0;
	return buffer;
}

char *otio_io_schema_names[] = {"Stack", //	ORIO_OT_TRACK
								"Sequence", //	ORIO_OT_SEQUENCE
								"Clip", //	ORIO_OT_CLIP
								"Filler", //	ORIO_OT_FILLER
								"Transition", //	ORIO_OT_TRANSITION
								"Effect", //	ORIO_OT_EFFECT
								"Marker", //	ORIO_OT_MARKER
								"MediaReference"}; //	ORIO_OT_MEDIA_REFFERENCE

extern void otio_save_node(FILE *f, OTIOObject *object, uint indentation);
								
void otio_save_list(FILE *f, OTIOObject *child, char *tabs, uint indentation, char *category)
{
	if(child != NULL)
	{
		fprintf(f, ",\n%s\t\"%s\" : [\n", tabs, category);
		otio_save_node(f, child, indentation + 2);
		child = otio_object_next_get(child);
		while(child != NULL)
		{
			fprintf(f, ",\n");
			otio_save_node(f, child, indentation + 2);
			child = otio_object_next_get(child);
		}
		fprintf(f, "\n%s\t]", tabs);
	}
}

void otio_save_time(FILE *f, char *tabs, char *name, OTIOTime time)
{
	fprintf(f, ",\n%s\"%s\": {", tabs, name);
	fprintf(f, "\n%s\t\"OTIO_SCHEMA\" : \"RationalTime.1\",", tabs);
	fprintf(f, "\n%s\t\"rate\" : %f,", tabs, time.time_scale);
	fprintf(f, "\n%s\t\"value\" : %f", tabs, time.time_value);
	fprintf(f, "\n%s}", tabs);
}


void otio_save_node(FILE *f, OTIOObject *object, uint indentation)
{
	char tabs[64];
	OTIOObjectType type;
	char *text;
	uint i;
	for(i = 0; i < 63 && i < indentation; i++)
		tabs[i] = '\t';
	tabs[i] = 0;
	type = otio_object_type_get(object);
	fprintf(f, "%s{\n", tabs);
	fprintf(f, "%s\t\"OTIO_SCHEMA\" : \"%s.%u\"", tabs, otio_io_schema_names[type], 1);
	fprintf(f, ",\n%s\t\"name\" : \"%s\"", tabs, otio_io_schema_names[otio_object_type_get(object)]);
	fprintf(f, ",\n%s\t\"metadata\" : {}", tabs);
	if(type < ORIO_OT_COMPOSABLE_END)
	{
		if(type != ORIO_OT_TRANSITION)
		{
			fprintf(f, ",\n%s\t\"source_range\": {", tabs);
			for(i = 0; i < 63 && i < indentation + 2; i++)
				tabs[i] = '\t';
			tabs[i] = 0;
			fprintf(f, "\n%s\"OTIO_SCHEMA\": \"TimeRange.1\"", tabs);
			otio_save_time(f, tabs, "duration", otio_composable_duration_time_get(object));
			otio_save_time(f, tabs, "start_time", otio_composable_start_time_get(object));
			for(i = 0; i < 63 && i < indentation; i++)
				tabs[i] = '\t';
			tabs[i] = 0;
			fprintf(f, "\n\t%s}", tabs);
		}
		if(type == ORIO_OT_CLIP)
			otio_save_list(f, otio_composable_children_get(object, 0), tabs, indentation, "media_reference");
		else
			otio_save_list(f, otio_composable_children_get(object, 0), tabs, indentation, "children");
		otio_save_list(f, otio_composable_effect_get(object, 0), tabs, indentation, "effects");
		otio_save_list(f, otio_composable_marker_get(object, 0), tabs, indentation, "markers");
	}
	switch(type)
	{
		case ORIO_OT_TRACK : 
		break;
		case ORIO_OT_SEQUENCE :
		break;
		case ORIO_OT_CLIP : 
		break;
		case ORIO_OT_FILLER :
		break;
		case ORIO_OT_TRANSITION :
			text = otio_transition_type_get(object);
			otio_save_time(f, tabs, "in_offset", otio_transition_in_time_get(object));
			otio_save_time(f, tabs, "out_offset", otio_transition_out_time_get(object));
			if(text != NULL)
				fprintf(f, ",\n%s\t\"type\" : \"%s\"", tabs, text);
		break;
		case ORIO_OT_EFFECT :
		break;
		case ORIO_OT_MARKER :		
			otio_save_time(f, tabs, "time", otio_marker_time_get(object));
			text = otio_marker_message_get(object);
			if(text != NULL)
				fprintf(f, ",\n%s\t\"message\" : \"%s\"", tabs, text);
		break;
		case ORIO_OT_MEDIA_REFFERENCE :
			otio_save_time(f, tabs, "available_range", otio_media_reference_available_range_get(object));

			text = otio_media_reference_uri_get(object);
			if(text != NULL)
				fprintf(f, ",\n%s\t\"kind\" : \"%s\"", tabs, text);
			text = otio_media_reference_kind_get(object);
			if(text != NULL)
				fprintf(f, ",\n%s\t\"uri\" : \"%s\"", tabs, text);
		break;
        case ORIO_OT_COUNT :
        break;
	}
	fprintf(f, "\n%s}", tabs);
}

int otio_save(char *path, OTIOObject *object)
{
	FILE *f;
	f = fopen(path, "w");
	if(f == NULL)
		return FALSE;
	otio_save_node(f, object, 0);
	fclose(f);
	return TRUE;
}

extern uint assemble_json_parse_object(void **link, char *string);

#define ORIO_OT_INTERNAL_RATIONAL_TIME ORIO_OT_COUNT
#define ORIO_OT_INTERNAL_TIME_RANGE (ORIO_OT_COUNT + 1)
#define ORIO_OT_INTERNAL_COUNT (ORIO_OT_COUNT + 2)

char *otio_load_schema_names[ORIO_OT_INTERNAL_COUNT] = {"Stack", // ORIO_OT_TRACK,
														"Sequence", // ORIO_OT_SEQUENCE,
														"Clip", // ORIO_OT_CLIP,
														"Filler", // ORIO_OT_FILLER,
														"Transition", // ORIO_OT_TRANSITION,
														"Effect", // ORIO_OT_EFFECT,
														"Marker", // ORIO_OT_MARKER,
														"MediaReference", // ORIO_OT_MEDIA_REFFERENCE,
														"RationalTime", // ORIO_OT_INTERNAL_RATIONAL_TIME
														"TimeRange"}; // ORIO_OT_INTERNAL_TIME_RANGE

uint otio_io_object_shema_get(AJsonObject *root, uint *version)
{
	AJsonValue	*value;
	char *string;
	uint i, j;
	value = assemble_json_object_member_search_name_get_value(root, "OTIO_SCHEMA", A_JT_STRING);
	if(value == NULL)
		return -1;
	string = assemble_json_value_get_string(value);
	for(i = 0; i < ORIO_OT_INTERNAL_COUNT; i++)
	{
		for(j = 0; string[j] == otio_load_schema_names[i][j] && otio_load_schema_names[i][j] != 0; j++);
		if(string[j] == '.' && otio_load_schema_names[i][j] == 0)
		{
			*version = 0;			
			for(j++; string[j] >= '0' && string[j] <= '9'; j++)
				*version = (*version * 10) + string[j] - '0';
			return i;
		}
	}
	return -1;
}

uint otio_io_rational_time_get(AJsonObject *root, char *name, OTIOTime *time)
{
	AJsonObject	*member;
	AJsonValue	*value;
	uint version;
	value = assemble_json_object_member_search_name_get_value(root, name, A_JT_OBJECT);
	if(value == NULL)
		return FALSE;
	member = assemble_json_value_get_object(value);
	if(ORIO_OT_INTERNAL_RATIONAL_TIME != otio_io_object_shema_get(member, &version))
		return FALSE;
	value = assemble_json_object_member_search_name_get_value(member, "rate", A_JT_NUMBER);
	if(value != NULL)
		time->time_scale = assemble_json_value_get_number_double(value);
	else
		return FALSE;
	value = assemble_json_object_member_search_name_get_value(member, "value", A_JT_NUMBER);
	if(value != NULL)
		time->time_value = assemble_json_value_get_number_double(value);
	else
		return FALSE;
    return TRUE;
}

uint otio_io_range_time_get(AJsonObject *root, char *name, OTIOTime *duration, OTIOTime *start_time)
{
	AJsonObject	*member;
	AJsonValue	*value;
	value = assemble_json_object_member_search_name_get_value(root, name, A_JT_OBJECT);
	if(value == NULL)
		return FALSE;
	member = assemble_json_value_get_object(value);
	if(!otio_io_rational_time_get(member, "duration", duration))
		return FALSE;
	if(!otio_io_rational_time_get(member, "start_time", start_time))
		return FALSE;
	return TRUE;
}

extern OTIOObject *otio_io_object_load(AJsonObject *root, uint expected);

void otio_io_object_load_children(OTIOObject *object, AJsonObject *root, char *name, uint expected)
{
	OTIOObject *child;
	AJsonValue	*value;
	value = assemble_json_object_member_search_name_get_value(root, name, A_JT_ARRAY);
	if(value != NULL)
	{
		for(value = assemble_json_value_get_array(value); value != NULL; value = assemble_json_value_get_next(value))
		{
			if(A_JT_OBJECT == assemble_json_value_get_type(value))
			{
				child = otio_io_object_load(assemble_json_value_get_object(value), expected);
				if(child != NULL)
					otio_object_insert_after(object, child, NULL); 	
			}
		}
	}
}

OTIOObject *otio_io_object_load(AJsonObject *root, uint expected)
{
	OTIOObject *object;
	AJsonValue	*value;
	OTIOTime time;
	uint type, version;
	char *name = "unnamed";
	type = otio_io_object_shema_get(root, &version);
	if(type > ORIO_OT_COUNT)
		return NULL;
	if(type < ORIO_OT_COMPOSABLE_END)
	{
		if(expected != 0)
			return NULL;
	}else if(expected != type)
		return NULL;
	value = assemble_json_object_member_search_name_get_value(root, "name", A_JT_STRING);
	if(value != NULL)
		name = assemble_json_value_get_string(value);
	object = otio_object_create(type, name);
	
	switch(type)
	{
		case ORIO_OT_TRACK : 
			otio_io_object_load_children(object, root, "children", 0);
		break;
		case ORIO_OT_SEQUENCE :
			otio_io_object_load_children(object, root, "children", 0);
		break;
		case ORIO_OT_CLIP : 
			otio_io_object_load_children(object, root, "media_reference", ORIO_OT_MEDIA_REFFERENCE);
		break;
		case ORIO_OT_FILLER :
		break;
		case ORIO_OT_TRANSITION :
			if(otio_io_rational_time_get(root, "in_offset", &time))
				otio_transition_in_time_set(object, time);
			if(otio_io_rational_time_get(root, "out_offset", &time))
				otio_transition_out_time_set(object, time);
			value = assemble_json_object_member_search_name_get_value(root, "type", A_JT_STRING);
			if(value != NULL)
				otio_transition_type_set(object, assemble_json_value_get_string(value));
		break;
		case ORIO_OT_EFFECT :
		break;
		case ORIO_OT_MARKER :
			if(otio_io_rational_time_get(root, "time", &time))
				otio_marker_time_set(object, time);
			value = assemble_json_object_member_search_name_get_value(root, "message", A_JT_STRING);
			if(value != NULL)
				otio_marker_message_set(object, assemble_json_value_get_string(value));
		break;
		case ORIO_OT_MEDIA_REFFERENCE :
			if(otio_io_rational_time_get(root, "available_range", &time))
				otio_media_reference_available_range_set(object, time);
			value = assemble_json_object_member_search_name_get_value(root, "kind", A_JT_STRING);
			if(value != NULL)
				otio_media_reference_kind_set(object, assemble_json_value_get_string(value));
			value = assemble_json_object_member_search_name_get_value(root, "uri", A_JT_STRING);
			if(value != NULL)
				otio_media_reference_uri_set(object, assemble_json_value_get_string(value));
		break;
	}
	if(type < ORIO_OT_COMPOSABLE_END)
	{
		OTIOTime duration, start_time;
		if(otio_io_range_time_get(root, "source_range", &duration, &start_time))
		{
			otio_composable_start_time_set(object, start_time);
			otio_composable_duration_time_set(object, duration);
		}
		otio_io_object_load_children(object, root, "effects", ORIO_OT_EFFECT);
		otio_io_object_load_children(object, root, "markers", ORIO_OT_MARKER);
	}
	return object;
}

extern uint assemble_json_print(char *array, AJsonValue *value, uint indentation);

OTIOObject *otio_load(char *string)
{
	char *array;
	AJsonValue	*value;
	uint pos, pos2;
	value = assemble_json_parse(string, TRUE);
	if(value == NULL)
		return NULL;
	pos = assemble_json_print_size(value, 0);
	array = malloc(pos + 1);
	assemble_json_print(array, value, 0);
	array[pos] = 0;
	printf("%s", array);
	free(array);
	if(A_JT_OBJECT != assemble_json_value_get_type(value))
		return NULL;
	return otio_io_object_load(assemble_json_value_get_object(value), 0);
}