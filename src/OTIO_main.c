#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "OTIO_internal.h"

//static char *otio_object_type_names[ORIO_OT_COUNT] = {"ORIO_OT_TRACK", "ORIO_OT_SEQUENCE", "ORIO_OT_CLIP", "ORIO_OT_FILLER", "ORIO_OT_TRANSITION", "ORIO_OT_EFFECT", "ORIO_OT_MARKER", "ORIO_OT_MEDIA_REFFERENCE"};


#ifdef OTIO_DEBUG_MODE
char *otio_object_type_names[ORIO_OT_COUNT] = {"ORIO_OT_TRACK", "ORIO_OT_SEQUENCE", "ORIO_OT_CLIP", "ORIO_OT_FILLER", "ORIO_OT_TRANSITION", "ORIO_OT_EFFECT", "ORIO_OT_MARKER", "ORIO_OT_MEDIA_REFFERENCE"};
#endif

char *otio_text_allocate(char *string) 
{
	static uint text_alloc = 0;
	uint i;
	char *copy;
	if(string == NULL)
		return NULL;
	for(i = 0; string[i] != 0; i++);
	copy = malloc(++i);
	memcpy(copy, string, i);
	return copy;
}

void otio_list_clear(OTIOList *list)
{
	list->first = NULL;
	list->last = NULL;
	list->count = 0;
}

OTIOHeader *otio_object_create(OTIOObjectType type, char *name)
{
	OTIOHeader *object;
	switch(type)
	{
		case ORIO_OT_TRACK :
		case ORIO_OT_SEQUENCE :
		case ORIO_OT_CLIP :
		case ORIO_OT_FILLER :
		{		
			OTIOComposable *composable;
			composable = malloc(sizeof *composable);
			object = (OTIOHeader *)composable;
			otio_list_clear(&composable->children);
			otio_list_clear(&composable->markers);
			otio_list_clear(&composable->effects);
			composable->start_time.time_scale = 1;
			composable->start_time.time_value = 0;
			composable->duration_time.time_scale = 1;	
			composable->duration_time.time_value = 0;	
		}
		break;
		case ORIO_OT_TRANSITION :
		{		
			OTIOTransition *transition;
			transition = malloc(sizeof *transition);
			object = (OTIOHeader *)transition;
			transition->transition_type = NULL;
			transition->in_time.time_scale = 1;
			transition->in_time.time_value = 1;
			transition->out_time.time_scale = 1;
			transition->out_time.time_value = 1;
			otio_list_clear(&transition->header.children);
			otio_list_clear(&transition->header.markers);
			otio_list_clear(&transition->header.effects);
			transition->header.start_time.time_scale = 1;
			transition->header.start_time.time_value = 0;
			transition->header.duration_time.time_scale = 1;	
			transition->header.duration_time.time_value = 0;
		}
		break;
		case ORIO_OT_EFFECT :
		{		
			OTIOEffect *effect;
			effect = malloc(sizeof *effect);
			object = (OTIOHeader *)effect;
		}
		break;
		case ORIO_OT_MARKER :
		{		
			OTIOMarker *marker;
			marker = malloc(sizeof *marker);
			object = (OTIOHeader *)marker;
			marker->time.time_scale = 1;
			marker->time.time_value = 0;
			marker->message = NULL;
		}
		break;
		case ORIO_OT_MEDIA_REFFERENCE :
		{		
			OTIOMediaReferences *media_reference;
			media_reference = malloc(sizeof *media_reference);
			object = (OTIOHeader *)media_reference;
			media_reference->kind = NULL;
			media_reference->uri = NULL;
			media_reference->available_range.time_scale = 1;
			media_reference->available_range.time_value = 0;
		}
		break;
	}
	object->name = otio_text_allocate(name);
	object->parent = NULL;
	object->meta = NULL;
	object->next = NULL;
	object->prev = NULL;
	object->type = type;
	object->metadata = NULL;
	object->user_data = NULL;
	return object;
}





OTIOObjectType otio_object_type_get(void *object) /* get the type of an object (if you lose track of what you are doing...) */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_type_get with a NULL pointer.\n");
		return ORIO_OT_COUNT;
	}
#endif
	return ((OTIOHeader *)object)->type;
}

char *otio_object_name_get(void *object) /* Get the name of an object. Pointer is valid until object is freed or name is set. */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_name_get with a NULL pointer.\n");
		return "Error";
	}
#endif
	return ((OTIOHeader *)object)->name;
}

void otio_object_name_set(void *object, char *name) /* Set the name of the object. */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
		printf("OTIO Error: Calling otio_object_name_set with a NULL pointer.\n");
	if(name == NULL)
		printf("OTIO Error: Calling otio_object_name_set with a NULL string.\n");
#endif
	if(((OTIOHeader *)object)->name)
		free(((OTIOHeader *)object)->name);
	((OTIOHeader *)object)->name = otio_text_allocate(name); 
}

void *otio_object_parent_get(void *object) /* Get the parent object, of one is available. */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_parent_get with a NULL pointer.\n");
		return NULL;
	}
#endif
	return ((OTIOHeader *)object)->parent;
}

void *otio_object_next_get(void *object) /* get the next object in the sequence. Return NULL if its the last object. */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_next_get with a NULL pointer.\n");
		return NULL;
	}
#endif
	return ((OTIOHeader *)object)->next;
}

void *otio_object_previous_get(void *object) /* get the next object in the sequence. Return NULL if its the last object. */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_previous_get with a NULL pointer.\n");
		return NULL;
	}
#endif
	return ((OTIOHeader *)object)->prev;
}

OTIOList *otio_object_parent_list_get(void *object) 
{
	if(((OTIOHeader *)object)->type == ORIO_OT_EFFECT)
		return &((OTIOComposable *)((OTIOHeader *)object)->parent)->effects;
	if(((OTIOHeader *)object)->type == ORIO_OT_MARKER)
		return &((OTIOComposable *)((OTIOHeader *)object)->parent)->markers;
	return &((OTIOComposable *)((OTIOHeader *)object)->parent)->children;
}

void *otio_object_list_lookup(OTIOList *list, uint id)
{
	uint i = 0;
	OTIOHeader *header;
	if(id >= list->count)
		return NULL;
	if(i <= list->count / 2)
	{
		for(header = list->first; i < id; header = header->next)
			i++;
		return header;
	}else
	{
		i = list->count - 1;
		for(header = list->last; i > id; header = header->prev);
			i--;
		return header;
	}
}

#ifdef OTIO_DEBUG_MODE
extern void otio_object_detach_internal(void *object); /* in debug mode we have an internal detach without confusing debug messages */
#else
#define otio_object_detach_internal otio_object_detach /* Outside of debug mode they are all the same */
#endif

void otio_object_detach(void *object) /* Detaches the object from its parent. */
{
#ifdef OTIO_DEBUG_MODE
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_detach with a NULL pointer.\n");
		return;
	}
	otio_object_detach_internal(object);
}

void otio_object_detach_internal(void *object) /* Detaches the object from its parent. */
{
	OTIOList *list;
#endif
	if(((OTIOHeader *)object)->parent == NULL)
		return;
	list = otio_object_parent_list_get(object);
	if(list->first == object)
		list->first = ((OTIOHeader *)object)->next;
	if(list->last == object)
		list->last = ((OTIOHeader *)object)->prev;
	list->count--;
	if(((OTIOHeader *)object)->next != NULL)
	{
		((OTIOHeader *)((OTIOHeader *)object)->next)->prev = ((OTIOHeader *)object)->prev;
		((OTIOHeader *)object)->next = NULL;
	}
	if(((OTIOHeader *)object)->prev != NULL)
	{
		((OTIOHeader *)((OTIOHeader *)object)->prev)->next = ((OTIOHeader *)object)->next;
		((OTIOHeader *)object)->prev = NULL;
	}
	((OTIOHeader *)object)->parent = NULL;
}


#ifdef OTIO_DEBUG_MODE
uint otio_object_insert_before_error_message(OTIOHeader *parent, OTIOHeader *object, OTIOHeader *before_after, char *when)
{
	if(object == NULL)
	{	
		printf("OTIO Error: Calling otio_object_insert_%s with a object NULL pointer.\n", when);
		return FALSE;
	}
	if(parent == NULL)
	{	
		printf("OTIO Error: Calling otio_object_insert_%s with a parent NULL pointer.\n", when);
		return FALSE;
	}

	if(before_after != NULL && before_after->parent != parent)
	{
		printf("OTIO Error: Calling otio_object_insert_%s with a %s parameter that isnt parented to the parent parameter object.\n", when, when);
		return FALSE;		
	}

	if(object->type < ORIO_OT_COMPOSABLE_END)
	{
		if(parent->type != ORIO_OT_TRACK && parent->type != ORIO_OT_SEQUENCE)
		{
			printf("OTIO Error: Calling otio_object_insert_%s A %s object can only be parented to either a track or sequence object, not a %s obeject\n", when, otio_object_type_names[object->type], otio_object_type_names[parent->type]);
			return FALSE;
		}
	}else
	{
		if(object->type == ORIO_OT_EFFECT && parent->type >= ORIO_OT_COMPOSABLE_END)
		{
			printf("OTIO Error: Calling otio_object_insert_%s A Effect object can only be parented to composable object\n", when);
			return FALSE;
		}
		if(object->type == ORIO_OT_MARKER && parent->type >= ORIO_OT_COMPOSABLE_END)
		{
			printf("OTIO Error: Calling otio_object_insert_%s A Marker object can only be parented to composable object\n", when);
			return FALSE;
		}
		if(object->type == ORIO_OT_MEDIA_REFFERENCE && parent->type == ORIO_OT_CLIP)
		{
			printf("OTIO Error: Calling otio_object_insert_%s A Media reference object can only be parented to clip objects\n", when);
			return FALSE;
		}
	}
	return TRUE;
}
#endif	

void otio_object_insert_before(OTIOHeader *parent, OTIOHeader *object, OTIOHeader *before_object) /* inserts a object in to a parent object. if before_object is NULL, the object will e placed last, other whise it will be placed before the before_object. The object will automaticaly be detached form any previous object.*/
{
	OTIOList *list;
#ifdef ITIO_DEBUG_MODE
	if(!otio_object_insert_before_error_message(parent, object, before_object, "before"))
		return;
#endif	
	otio_object_detach_internal(object);
	object->parent = parent;
	list = otio_object_parent_list_get(object);
	list->count++;
	if(before_object == NULL)
	{
		if(list->first == NULL)
		{
			list->first = list->last = object;
			list->count = 1;
		}else
		{
			object->next = list->first;
			((OTIOHeader *)list->first)->prev = object;
			list->first = object;
		}
	}else
	{
		object->prev = before_object->prev;
		before_object->prev = object;
		if(object->prev == NULL)
			list->first = object;
		else
			((OTIOHeader *)object->prev)->next = object;
		object->next = before_object;
	}
}

void otio_object_insert_after(OTIOHeader *parent, OTIOHeader *object, OTIOHeader *after_object) /* inserts a object in to a parent object. if after_object is NULL, the object will e placed first, other whise it will be placed after the after_object. The object will automaticaly be detached form any previous object.*/
{
	OTIOList *list;
#ifdef ITIO_DEBUG_MODE
	if(!otio_object_insert_before_error_message(parent, object, after_object, "after"))
		return;
#endif	
	otio_object_detach_internal(object);
	object->parent = parent;
	list = otio_object_parent_list_get(object);
	list->count++;
	if(after_object == NULL)
	{
		if(list->first == NULL)
		{
			list->first = list->last = object;
			list->count = 1;
		}else
		{
			object->prev = list->last;
			((OTIOHeader *)list->last)->next = object;
			list->last = object;
		}
	}else
	{
		object->next = after_object->next;
		after_object->next = object;
		if(object->next == NULL)
			list->last = object;
		else
			((OTIOHeader *)object->next)->prev = object;
		object->prev = after_object;
	}
}

extern void otio_object_free(void *object);

void otio_object_list_free(OTIOList *list) /* free an object, and its children. if an object is attached to a a parrent it will be detached automaticaly. */
{
	while(list->first != NULL)
		otio_object_free(list->first);
}

void otio_object_free(OTIOHeader *object) /* free an object, and its children. if an object is attached to a a parrent it will be detached automaticaly. */
{
	otio_object_detach_internal(object);
	if(object->name != NULL)
		free(object->name);
	switch(object->type)
	{
		case ORIO_OT_TRACK :
		case ORIO_OT_SEQUENCE :
		case ORIO_OT_CLIP :
		case ORIO_OT_FILLER :
		case ORIO_OT_TRANSITION :
		{		
			OTIOComposable *composable;
			composable = (OTIOComposable *)object;
			otio_object_list_free(&composable->children);
			otio_object_list_free(&composable->markers);
			otio_object_list_free(&composable->effects);
		}
		break;
		case ORIO_OT_EFFECT :
		{		
			OTIOEffect *effect;
			effect = (OTIOEffect *)object;
		}
		break;
		case ORIO_OT_MARKER :
		{		
			OTIOMarker *marker;
			marker = (OTIOMarker *)object;
			if(marker->message != NULL)
				free(marker->message);
		}
		break;
		case ORIO_OT_MEDIA_REFFERENCE :
		{		
			OTIOMediaReferences *media_reference;
			media_reference = (OTIOMediaReferences *)object;
			if(media_reference->kind != NULL)
				free(media_reference->kind);
			if(media_reference->uri != NULL)
				free(media_reference->uri);
		}
		break;
	}
	free(object);
}

void otio_object_user_data_set(OTIOHeader *object, void *user_data)
{
	object->user_data = user_data;
}

void *otio_object_user_data_get(OTIOHeader *object)
{
	return object->user_data;
}

uint otio_composable_children_count(OTIOHeader *object)
{
	if(object->type >= ORIO_OT_COMPOSABLE_END)
		return 0;
	return ((OTIOComposable *)object)->children.count;
}

OTIOHeader *otio_composable_children_get(OTIOHeader *object, uint id)
{
	OTIOHeader *out;
	uint i;
	if(object->type >= ORIO_OT_COMPOSABLE_END)
		return NULL;
	out = ((OTIOComposable *)object)->children.first;
	for(i = 0; i < id && out != NULL; i++)
		out = out->next;
	return out;
}

uint otio_composable_markers_count(OTIOHeader *object)
{
	if(object->type >= ORIO_OT_COMPOSABLE_END)
		return 0;
	return ((OTIOComposable *)object)->markers.count;
}

OTIOHeader *otio_composable_marker_get(OTIOHeader *object, uint id)
{
	OTIOHeader *out;
	uint i;
	if(object->type >= ORIO_OT_COMPOSABLE_END)
		return NULL;
	out = ((OTIOComposable *)object)->markers.first;
	for(i = 0; i < id && out != NULL; i++)
		out = out->next;
	return out;
}

uint otio_composable_effect_count(OTIOHeader *object)
{
	if(object->type >= ORIO_OT_COMPOSABLE_END)
		return 0;
	return ((OTIOComposable *)object)->effects.count;
}

OTIOHeader *otio_composable_effect_get(OTIOHeader *object, uint id)
{
	OTIOHeader *out;
	uint i;	
	if(object->type >= ORIO_OT_COMPOSABLE_END)
		return NULL;
	out = ((OTIOComposable *)object)->effects.first;
	for(i = 0; i < id && out != NULL; i++)
		out = out->next;
	return out;
}


OTIOTime otio_composable_start_time_get(OTIOHeader *object)
{
	if(object->type >= ORIO_OT_COMPOSABLE_END)
	{
		OTIOTime time;
		time.time_scale = 0;
		time.time_scale = 0;
		return time;
	}
	return ((OTIOComposable *)object)->start_time;
}

void otio_composable_start_time_set(OTIOHeader *object, OTIOTime start_time)
{
	if(object->type < ORIO_OT_COMPOSABLE_END)
		((OTIOComposable *)object)->start_time = start_time;
}

OTIOTime otio_composable_duration_time_get(OTIOHeader *object)
{
	if(object->type >= ORIO_OT_COMPOSABLE_END)
	{
		OTIOTime time;
		time.time_scale = 0;
		time.time_scale = 0;
		return time;
	}
	return ((OTIOComposable *)object)->duration_time;
}

void otio_composable_duration_time_set(OTIOHeader *object, OTIOTime duration)
{
	if(object->type < ORIO_OT_COMPOSABLE_END)
		((OTIOComposable *)object)->duration_time = duration;
}

char *otio_media_reference_kind_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_MEDIA_REFFERENCE)
		return "";
	return ((OTIOMediaReferences *)object)->kind;
}

void otio_media_reference_kind_set(OTIOHeader *object, char *kind)
{
	if(object->type != ORIO_OT_MEDIA_REFFERENCE)
		return;
	if(((OTIOMediaReferences *)object)->kind != NULL)
		free(((OTIOMediaReferences *)object)->kind);
	((OTIOMediaReferences *)object)->kind = otio_text_allocate(kind);
}


char *otio_media_reference_uri_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_MEDIA_REFFERENCE)
		return "";
	return ((OTIOMediaReferences *)object)->uri;
}

void otio_media_reference_uri_set(OTIOHeader *object, char *uri)
{
	if(object->type != ORIO_OT_MEDIA_REFFERENCE)
		return;
	if(((OTIOMediaReferences *)object)->uri != NULL)
		free(((OTIOMediaReferences *)object)->uri);
	((OTIOMediaReferences *)object)->uri = otio_text_allocate(uri);
}


OTIOTime otio_media_reference_available_range_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_MEDIA_REFFERENCE)
	{
		OTIOTime time;
		time.time_scale = 0;
		time.time_scale = 0;
		return time;
	}
	return ((OTIOMediaReferences *)object)->available_range;
}

void otio_media_reference_available_range_set(OTIOHeader *object, OTIOTime available_range)
{
	if(object->type == ORIO_OT_MEDIA_REFFERENCE)
		((OTIOMediaReferences *)object)->available_range = available_range;
}

char *otio_transition_type_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_TRANSITION)
		return "";
	return ((OTIOTransition *)object)->transition_type;
}

void otio_transition_type_set(OTIOHeader *object, char *type)
{
	if(object->type != ORIO_OT_TRANSITION)
		return;
	if(((OTIOTransition *)object)->transition_type != NULL)
		free(((OTIOTransition *)object)->transition_type);
	((OTIOTransition *)object)->transition_type = otio_text_allocate(type);
}


OTIOTime otio_transition_in_time_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_TRANSITION)
	{
		OTIOTime time;
		time.time_scale = 0;
		time.time_scale = 0;
		return time;
	}
	return ((OTIOTransition *)object)->in_time;
}

void otio_transition_in_time_set(OTIOHeader *object, OTIOTime in_time)
{
	if(object->type == ORIO_OT_TRANSITION)
		((OTIOTransition *)object)->in_time = in_time;
}

OTIOTime otio_transition_out_time_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_TRANSITION)
	{
		OTIOTime time;
		time.time_scale = 0;
		time.time_scale = 0;
		return time;
	}
	return ((OTIOTransition *)object)->in_time;
}

void otio_transition_out_time_set(OTIOHeader *object, OTIOTime out_time)
{
	if(object->type == ORIO_OT_TRANSITION)
		((OTIOTransition *)object)->out_time = out_time;
}

OTIOTime otio_marker_time_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_MARKER)
	{
		OTIOTime time;
		time.time_scale = 0;
		time.time_scale = 0;
		return time;
	}
	return ((OTIOMarker *)object)->time;
}

void otio_marker_time_set(OTIOHeader *object, OTIOTime time)
{
	if(object->type == ORIO_OT_MARKER)
		((OTIOMarker *)object)->time = time;
}


char *otio_marker_message_get(OTIOHeader *object)
{
	if(object->type != ORIO_OT_MARKER)
		return "";
	return ((OTIOMarker *)object)->message;
}

void otio_marker_message_set(OTIOHeader *object, char *message)
{
	if(object->type != ORIO_OT_MARKER)
		return;
	if(((OTIOMarker *)object)->message != NULL)
		free(((OTIOMarker *)object)->message);
	((OTIOMarker *)object)->message = otio_text_allocate(message);
}

OTIOHeader *otio_object_clone(OTIOHeader *object, uint recursibely);

void otio_object_clone_list(OTIOHeader *parent, OTIOList *from_list, OTIOList *to_list)
{
	OTIOHeader *object, *clone;
	if(from_list->first != NULL)
	{
		object = from_list->first;
		clone = otio_object_clone(object, TRUE);
		clone->parent = parent;
		to_list->first = to_list->last = clone;
		for(object = object->next; object != NULL; object = object->next)
		{
			clone = otio_object_clone(object, TRUE);
			clone->parent = parent;
			clone->prev = to_list->last;
			to_list->last = clone;
		}
		to_list->count = from_list->count;
	}
}

OTIOHeader *otio_object_clone(OTIOHeader *object, uint recursively)
{
	OTIOHeader *clone;
	clone = otio_object_create(object->type, object->name);
	switch(object->type)
	{
		case ORIO_OT_TRANSITION :
		{		
			OTIOTransition *t_old, *t_new;
			t_old = (OTIOTransition *)object;
			t_new = (OTIOTransition *)clone;
			if(t_old->transition_type != NULL)
				t_new->transition_type = otio_text_allocate(t_old->transition_type);
			t_new->in_time = t_old->in_time;
			t_new->out_time = t_old->out_time;
		}
		case ORIO_OT_TRACK :
		case ORIO_OT_SEQUENCE :
		case ORIO_OT_CLIP :
		case ORIO_OT_FILLER :
		{		
			OTIOComposable *c_old, *c_new;
			c_old = (OTIOComposable *)object;
			c_new = (OTIOComposable *)clone;
			if(recursively)
			{
				otio_object_clone_list(object, &c_old->children, &c_new->children);
				otio_object_clone_list(object, &c_old->markers, &c_new->markers);
				otio_object_clone_list(object, &c_old->effects, &c_new->effects);
			}
			c_new->start_time = c_old->start_time;
			c_new->duration_time = c_old->duration_time;
		}
		break;
		case ORIO_OT_EFFECT :
		break;
		case ORIO_OT_MARKER :
		{		

			OTIOMarker *m_old, *m_new;
			m_old = (OTIOMarker *)object;
			m_new = (OTIOMarker *)clone;
			m_new->time = m_old->time;
			if(m_old->message != NULL)
				m_new->message = otio_text_allocate(m_old->message);
		}
		break;
		case ORIO_OT_MEDIA_REFFERENCE :
		{		
			OTIOMediaReferences *mr_old, *mr_new;
			mr_old = (OTIOMediaReferences *)object;
			mr_new = (OTIOMediaReferences *)clone;
			if(mr_old->kind != NULL)
				mr_new->kind = otio_text_allocate(mr_old->kind);
			if(mr_old->uri != NULL)
				mr_new->uri = otio_text_allocate(mr_old->uri);
			mr_old->available_range = mr_new->available_range;
		}
		break;
	}
	return clone;
}