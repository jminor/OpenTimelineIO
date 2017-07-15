
#define OTIO_INTERNAL

#include "OpenTimelineIO.h"


#define FALSE 0
#define TRUE !FALSE

typedef unsigned int uint;

typedef struct{
	void *first;
	void *last;
	uint count;
}OTIOList;

typedef struct{
	char *name;
	void *parent;
	void *meta;
	void *next;
	void *prev;
	OTIOObjectType type;
	void *metadata;
	void *user_data;
}OTIOHeader;

typedef struct{
	OTIOHeader header;
	OTIOList children;
	OTIOList markers;
	OTIOList effects;
	OTIOTime start_time;
	OTIOTime duration_time;
}OTIOComposable;

typedef struct{
	OTIOHeader header;
	char *kind;
	char *uri;
	OTIOTime available_range;
}OTIOMediaReferences;

typedef struct{
	OTIOComposable header;
	char *transition_type;
	OTIOTime in_time;
	OTIOTime out_time;
}OTIOTransition;

typedef struct{
	OTIOHeader header;
	OTIOTime time;
	char *message;
}OTIOMarker;

typedef struct{
	OTIOHeader header;
}OTIOEffect;
