
#ifndef uint
typedef unsigned int uint;
#endif

typedef enum{
	A_JT_STRING,
	A_JT_NUMBER,
	A_JT_OBJECT,
	A_JT_ARRAY,
	A_JT_TRUE,
	A_JT_FALSE,
	A_JT_NULL,
	A_JT_COUNT
}AJsonType;

typedef void AJsonObject;
typedef void AJsonValue;

extern AJsonValue	*assemble_json_parse(char *string, int write_protected);
extern uint			assemble_json_print_size(AJsonValue *value, uint indentation);
extern uint			assemble_json_print(char *array, AJsonValue *value, uint indentation);
extern char			*assemble_json_print_allocate(AJsonValue *value);

/* Object API */

extern AJsonObject	*assemble_json_object_member_get_next(AJsonObject *object);
extern char			*assemble_json_object_member_get_name(AJsonObject *object);
extern AJsonObject	*assemble_json_object_member_search_name_get_object(AJsonObject *object, char *name, uint type);
extern AJsonValue	*assemble_json_object_member_search_name_get_value(AJsonObject *object, char *name, uint type);
extern AJsonValue	*assemble_json_object_member_get_value(AJsonObject *object);

/* Value API */

extern AJsonValue	*assemble_json_clone(AJsonValue *value);

extern AJsonType	assemble_json_value_get_type(AJsonValue *value);
extern char			*assemble_json_value_get_string(AJsonValue *value);
extern double		assemble_json_value_get_number_double(AJsonValue *value);
extern float		assemble_json_value_get_number_float(AJsonValue *value);
extern AJsonObject	*assemble_json_value_get_object(AJsonValue *value);
extern AJsonValue	*assemble_json_value_get_array(AJsonValue *value);
extern AJsonValue	*assemble_json_value_get_next(AJsonValue *value);
extern void			assemble_json_value_object_add_string(AJsonValue *value, char *name, char *string);