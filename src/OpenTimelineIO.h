
/* C implementation of OpenTimlineIO format being created by Pixar Animation studios. Written by Eskil Steenberg (eskil 'at' quelsolaar 'dot' com). Please get in touch with any feedback you may have. Not that the Format has yer to be finalized so this implementation will change.*/

#ifndef OTIO_h
#define OTIO_h

#include <stdio.h>


/* Enable OTIO_DEBUG_MODE to output developer Error messages to standard out. */

#define OTIO_DEBUG_MODE

#ifndef uint
typedef unsigned int uint;
#endif
#ifndef uint8
typedef unsigned char uint8;
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

/* enum for all the types of objects that OTIO can store: */

typedef enum{
	ORIO_OT_TRACK,
	ORIO_OT_SEQUENCE,
	ORIO_OT_CLIP,
	ORIO_OT_FILLER,
	ORIO_OT_TRANSITION,
	ORIO_OT_EFFECT,
	ORIO_OT_MARKER,
	ORIO_OT_MEDIA_REFFERENCE,
	ORIO_OT_COUNT,
}OTIOObjectType;

#define ORIO_OT_COMPOSABLE_END (ORIO_OT_TRANSITION + 1) /* All OTIOObjectType values LESS then this value are "composable" */

static char *otio_object_type_names[ORIO_OT_COUNT]; /* An array of strings corresponding to the names of the various object types. Handy for debugging printouts.*/

typedef struct {
	double time_value; /* Number of frames / samples (may not be an integer) */
    double time_scale; /* Number of frames per second, again may not be an even number Example 29.97 FPS*/
}OTIOTime;

#ifndef OTIO_INTERNAL

typedef void OTIOObject; /* Markers can be stored in Tracks or Timelines.*/

/* Common functionallity that can be used on all objects (Timelines, Tracks, Sequences, Clips, Transitions, Effects and Markers) */

extern OTIOObject	*otio_load(char *string); /* takes a string conmtaining the Json Description of the OpenTimelineIO file, and returns the root object of the structure */
extern int			otio_save(char *path, OTIOObject *object); /* Takes a Object and saves it to a file named "path" */
extern OTIOObject	*otio_object_create(OTIOObjectType type, char *name); /* Creates a new empty object of type "type" with the name "name". */
extern OTIOObject	*otio_object_clone(OTIOObject *object, uint recursively); /* Clones an existing object. If "recursivly" is set to TRUE all child objects, markers and effects will aslo be cloned, createing a separate object tree.*/
extern void			otio_object_free(OTIOObject *object); /* free an object, and its children. if an object is attached to a a parrent it will be detached automaticaly. */


extern OTIOObjectType otio_object_type_get(OTIOObject *object); /* get the type of an object (if you lose track of what you are doing...) */
extern char			*otio_object_name_get(OTIOObject *object); /* Get the name of an object. Pointer is valid until object is freed or name is set. */
extern void			otio_object_name_set(OTIOObject *object, char *name); /* Set the name of the object. */
extern void			*otio_object_parent_get(OTIOObject *object); /* Get the parent object, of one is available. */
extern void			*otio_object_next_get(OTIOObject *object); /* get the next object in the sequence. Return NULL if its the last object. */
extern void			*otio_object_previous_get(OTIOObject *object); /* get the next object in the sequence. Return NULL if its the last object. */
extern void			otio_object_detach(OTIOObject *object); /* Detaches the object from its parent. */
extern void			otio_object_insert_before(OTIOObject *parent, OTIOObject *object, OTIOObject *before_object); /* inserts a object in to a parent object. if before_object is NULL, the object will e placed last, other whise it will be placed before the before_object. The object will automaticaly be detached form any previous object.*/
extern void			otio_object_insert_after(OTIOObject *parent, OTIOObject *object, OTIOObject *after_object); /* inserts a object in to a parent object. if after_object is NULL, the object will e placed first, other whise it will be placed after the after_object. The object will automaticaly be detached form any previous object.*/
extern void			otio_object_user_data_set(OTIOObject *object, void *user_data); /* Attach a pointer ofd user data to an object. This can be anything you want to conviniently store with the object. Remember that you manualy have to free the data before deleting the object. */
extern void			*otio_object_user_data_get(OTIOObject *object); /* Get the attached user pointer. */


/* Functions only applicable to composable objects. (ORIO_OT_TRACK, ORIO_OT_SEQUENCE, ORIO_OT_CLIP, ORIO_OT_FILLER, and ORIO_OT_TRANSITION) */

/* ORIO_OT_TRACK and ORIO_OT_SEQUENCE objects can have childeren of any composable type. ORIO_OT_CLIP objects can onnly have children of ORIO_OT_MEDIA_REFFERENCE type. */

extern uint			otio_composable_children_count(OTIOObject *object); /* Get the number of children of a objects. */
extern OTIOObject	*otio_composable_children_get(OTIOObject *object, uint id); /* Get the a child of a objects. It is recomended for performance to use this function to get the first child, then use otio_object_next_get to iterate over the children rather then using the number param. */

extern uint			otio_composable_marker_count(OTIOObject *object);/* Get the number of markers on the object. */
extern OTIOObject	*otio_composable_marker_get(OTIOObject *object, uint id); /* Get the a marker on the object. It is recomended for performance to use this function to get the first marker, then use otio_object_next_get to iterate over the markers rather then using the number param. */

extern uint			otio_composable_effect_count(OTIOObject *object);/* Get the number of effects on the object. */
extern OTIOObject	*otio_composable_effect_get(OTIOObject *object, uint id); /* Get the a effect on the object. It is recomended for performance to use this function to get the first effect, then use otio_object_next_get to iterate over the effects rather then using the number param. */

extern OTIOTime		otio_composable_start_time_get(OTIOObject *object); /* Get the start time of the composable object. */
extern void			otio_composable_start_time_set(OTIOObject *object, OTIOTime start_time); /* Set the start time of the composable object. */
extern OTIOTime		otio_composable_duration_time_get(OTIOObject *object);/* Get the duration time of the composable object. */
extern void			otio_composable_duration_time_set(OTIOObject *object, OTIOTime duration);/* Set the duration time of the composable object. */

/* Functions only applicable to objects of the type ORIO_OT_MEDIA_REFFERENCE. Media reference objects can only be children to Clips */

#define OTIT_MEDIA_REFERENCE_KIND_STORYBOARD "Storyboard" /* These defines are place holders I made up for now.*/
#define OTIT_MEDIA_REFERENCE_KIND_PREVIS "Previs" 
#define OTIT_MEDIA_REFERENCE_KIND_RUSHES "Rushes" 
#define OTIT_MEDIA_REFERENCE_KIND_FINAL "Final" 
#define OTIT_MEDIA_REFERENCE_KIND_LEFT_EYE "Left Eye" 
#define OTIT_MEDIA_REFERENCE_KIND_RIGHT_EYE "Right Eye" 

extern char			*otio_media_reference_kind_get(OTIOObject *object); /* Get reference kind string */
extern void			otio_media_reference_kind_set(OTIOObject *object, char *kind); /* Set reference kind string */
extern char			*otio_media_reference_uri_get(OTIOObject *object); /* Get reference uri */
extern void			otio_media_reference_uri_set(OTIOObject *object, char *uri); /* Set reference uri */
extern OTIOTime		otio_media_reference_available_range_get(OTIOObject *object); /* Get the available range of the media */
extern void			otio_media_reference_available_range_set(OTIOObject *object, OTIOTime available_range);/* Set the available range of the media */

/* Functions only applicable to objects of the type ORIO_OT_TRANSITION. Trasnitions are composable but, always have length 0. */

#define OTIT_TRANSITION_TYPE_SMPTE_DISOLVE "SMPTE_Dissolve" /* Standard name for Disolve, more of these to come*/

extern char			*otio_transition_type_get(OTIOObject *object); /* Get Text string to Describe the type of transition */
extern void			otio_transition_type_set(OTIOObject *object, char *type); /* Set Text string to Describe the type of transition */
extern OTIOTime		otio_transition_in_time_get(OTIOObject *object); /* Get the time the transition encrotches on the previous composable objects. */
extern void			otio_transition_in_time_set(OTIOObject *object, OTIOTime in_time); /* Set the time the transition encrotches on the previous composable objects. */
extern OTIOTime		otio_transition_out_time_get(OTIOObject *object); /* Get the time the transition encrotches on the following composable objects. */
extern void			otio_transition_out_time_set(OTIOObject *object, OTIOTime out_time); /* Set the time the transition encrotches on the following composable objects. */

/* Functions only applicable to objects of the type ORIO_OT_MARKER */

extern OTIOTime		otio_marker_time_get(OTIOObject *object); /* get the time stamp of the marker relative to its parents start time */
extern void			otio_marker_time_set(OTIOObject *object, OTIOTime time); /* set the time stamp of the marker relative to its parents start time */
extern char			*otio_marker_message_get(OTIOObject *object); /* get the maker message string */
extern void			 otio_marker_message_set(OTIOObject *object, char *message); /* set the maker message string */

#endif /* OTIO_INTERNAL */
#endif /* OTIO_h */