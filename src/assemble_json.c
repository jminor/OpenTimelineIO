#include <stdlib.h>
#include <stdio.h>

//#include "forge.h"

#define FALSE 0
#define TRUE !FALSE
typedef unsigned int uint;
typedef unsigned char uint8;


#define ASSEMBLE_JSON_DOUBLE_PRECISION

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

#define ASSEMBLE_JSON_TYPE_MASK (0x01 | 0x02 | 0x04) /* 3 first bits reserved for type*/
#define ASSEMBLE_JSON_ARRAY 0x08
#define ASSEMBLE_JSON_ROOT 0x10 
#define ASSEMBLE_JSON_WRITE_PROTECTED 0x20 


#ifdef ASSEMBLE_JSON_DOUBLE_PRECISION /* its possible to reduce the presision of numbers to 32 bits. may reduce mamory use and iuncrease speed, especialy on 32 bit machines */
	typedef double AJasonNumberType;
#else
	typedef float AJasonNumberType;
#endif

typedef struct {
	AJsonType type;
	union{
		char *string;
		AJasonNumberType number;
		void *object;
		void *array;
	}data;
}AJsonValue;

typedef struct {
	AJsonValue value;
	void *next;
}AJsonValueArray;

typedef struct {
	char *name;
	void *next;
	AJsonValue value;
}AJsonObject;

#define ASSEMBLE_JSON_VALUE_ALIGNMENT (sizeof(AJasonNumberType) > sizeof(void *) ? sizeof(AJasonNumberType) : sizeof(void *))

extern uint assemble_json_parse_object(void **link, char *string, uint8 **memory);

uint a = __alignof(AJsonObject);
uint b = _Alignof(AJsonObject);

uint assemble_parce_real(char *text, AJasonNumberType *real_output)
{
	double out = 0;
	double divider = 0.1, neg = 1.0;
	uint i = 0;
	if(text[0] == '-')
	{
		neg = -1.0;
		i = 1;
	}		
	while(TRUE)
	{
		if(text[i] >= '0' && text[i] <= '9')
		{
			out *= 10.0;
			out += text[i] - '0';
		}else
		{
			if(text[i] == '.')
				break;
			*real_output = out * neg;
			return i;
		}
		i++;
	}
	i++;
	while(TRUE)
	{
		if(text[i] >= '0' && text[i] <= '9')
			out += divider * (AJasonNumberType)(text[i] - '0');
		else
		{
			*real_output = out * neg;
			return i;
		}
		divider /= 10.0;
		i++;
	}
}

uint assemble_json_parse_value(AJsonValue *value, char *string, uint8 **memory)
{
	static uint value_allocate = 0;
	uint pos = 1;
	if(string[0] == '{')
	{
		if(value != NULL)
		{
			value->type = A_JT_OBJECT | ASSEMBLE_JSON_WRITE_PROTECTED;
			value->data.object = NULL;
			return 1 + assemble_json_parse_object(&value->data.object, &string[1], memory);
		}else		
			return 1 + assemble_json_parse_object(NULL, &string[1], memory);
	}else if(string[0] == '[')
	{	
		AJsonValueArray *v = NULL;
		uint add;
		if(value != NULL)
		{
			value->type = A_JT_ARRAY | ASSEMBLE_JSON_WRITE_PROTECTED;
			value->data.array = NULL;
		}
		while(string[pos] != ']' && string[pos] != '}')
		{	
			if(string[pos] > ' ' && string[pos] != ',')
			{		
				*memory += (uint)(*memory + sizeof(AJsonValue) - 1) % sizeof(AJsonValue); /* Alignment magic */
				if(value != NULL)
				{
					if(v != NULL)
					{	
						v->next = *memory;
						v = v->next;
					}else
						v = value->data.array = *memory;
					*memory += sizeof(AJsonValueArray);
					add = assemble_json_parse_value(&v->value, &string[pos], memory);
					if(add == -1)
						return -1;
					v->value.type |= ASSEMBLE_JSON_ARRAY;
					v->next = NULL;
					pos += add;
				}else
				{
					*memory += sizeof(AJsonValueArray);
					add = assemble_json_parse_value(NULL, &string[pos], memory);
					if(add == -1)
						return -1;
					pos += add;			
				}
			}
			pos++;
		}
		return pos;
	}else if(string[0] == '\"')
	{	
		uint i;
		if(value == NULL)
		{
			for(i = 1; string[i] != '"'; i++)
				if(string[i] == '\\')
					i++;
			*memory += i;
			pos += i;
		}else
		{
			value->data.string = *memory;
			for(i = 0; string[pos] != '"';)
			{
				if(string[pos] == '\\')
					value->data.string[i++] = string[pos++];
				value->data.string[i++] = string[pos++];
			}
			value->data.string[i++] = 0;
			*memory += i;
			value->type = A_JT_STRING | ASSEMBLE_JSON_WRITE_PROTECTED;
		}
		return pos;
	}else if((string[0] >= '0' && string[0] <= '9') || string[0] == '.' || string[0] == '-')
	{	
		if(value != NULL)
		{
			value->type = A_JT_NUMBER | ASSEMBLE_JSON_WRITE_PROTECTED;
			return assemble_parce_real(string, &value->data.number);
		}else
		{
			while((string[pos] >= '0' && string[pos] <= '9') || string[pos] == '.')
				pos++;
			return pos;
		}
	}else if(string[0] == 't' && string[1] == 'r' && string[2] == 'u' && string[3] == 'e' && (string[4] <= ' ' || string[4] == ',' || string[4] == ']' || string[4] == '}'))
	{			
		if(value != NULL)
			value->type = A_JT_TRUE | ASSEMBLE_JSON_WRITE_PROTECTED;
		return 4;
	}else if(string[0] == 'f' && string[1] == 'a' && string[2] == 'l' && string[3] == 's' && string[4] == 'e' && (string[5] <= ' ' || string[5] == ',' || string[5] == ']' || string[5] == '}'))
	{	
		if(value != NULL)
			value->type = A_JT_FALSE | ASSEMBLE_JSON_WRITE_PROTECTED;
		return 5;
	}else if(string[0] == 'n' && string[1] == 'u' && string[2] == 'l' && string[3] == 'l' && (string[4] <= ' ' || string[4] == ',' || string[4] == ']' || string[4] == '}'))
	{	
		if(value != NULL)
			value->type = A_JT_NULL | ASSEMBLE_JSON_WRITE_PROTECTED;
		return 4;
	}else
	{
		printf("JSON: Error: Unexpected end of file \n");
		return -1;
	}
}


uint assemble_json_parse_object(void **link, char *string, uint8 **memory)
{
	static uint obj_allocate = 0;
	AJsonObject *obj, *prev = NULL;
	uint i, j;
	for(i = 0; string[i] != 0 && string[i] != '}'; i++)
	{
		if(string[i] == '\"')
		{
			*memory += ((size_t)*memory + ASSEMBLE_JSON_VALUE_ALIGNMENT - 1) % ASSEMBLE_JSON_VALUE_ALIGNMENT; /* Alignment magic */
			if(link != NULL)
			{
				obj = (AJsonObject *)*memory;
				*memory += sizeof(AJsonObject);
				obj->next = NULL;
				if(prev == NULL)
					*link = obj;
				else
					prev->next = obj;
				prev = obj;
				obj->value.type = A_JT_NULL | ASSEMBLE_JSON_WRITE_PROTECTED;
				i++;
				obj->name = *memory;
				for(j = 0; string[i] != '"';)
				{
					if(string[i] == '\\')
						obj->name[j++] = string[i++];
					obj->name[j++] = string[i++];
				}
				i++;
				obj->name[j++] = 0;
				*memory += j;
			}else
			{		
				*memory += sizeof(AJsonObject);
				i++;
				for(j = 0; string[i + j] != '"' && string[i + j] != 0; j++)
					if(string[i + j] == '\\')
						j++;
				if(string[i + j] == 0)
					return -1;
				j++;
				*memory += j;
				i += j;
			}
		
			while(string[i] <= ' ' && string[i] != 0)
				i++;
			if(string[i] != ':')
			{
				if(string[i] == 0)
				{	
					printf("JSON: Error: Unexpected end of file after \"%s\"\n", obj->name);
					return -1;
				}
				printf("JSON: Error: Expected : after \"%s\"\n", obj->name);
				return -1;
			}
			i++;
			while(string[i] <= ' ' && string[i] != 0)
				i++;
			if(string[i] == 0)
			{	
				printf("JSON: Error: Unexpected end of file after \"%s\"\n", obj->name);
				return -1;
			}
			if(link == NULL)
				j = assemble_json_parse_value(NULL, &string[i], memory);
			else
				j = assemble_json_parse_value(&obj->value, &string[i], memory);
			if(j == -1)
				return -1;
			i += j;
		}else if(string[i] > ' ' && string[i] != ',')
		{
			printf("JSON: Error: Unexpected end of file after \"%s\"\n", obj->name);
			return -1;	
		}
	}
	return i;
}


void assemble_json_clone_internal(AJsonValue *from_value, AJsonValue *to_value)
{
	to_value->type = from_value->type & ASSEMBLE_JSON_TYPE_MASK;
	switch(to_value->type)
	{
		case A_JT_STRING :
		{
			uint i;
			for(i = 0; from_value->data.string[i] != 0; i++);
			to_value->data.string = malloc((sizeof *to_value->data.string) * ++i);
			for(i = 0; from_value->data.string[i] != 0; i++)
				to_value->data.string[i] = from_value->data.string[i];
			to_value->data.string[i] = 0;
		}break;
		case A_JT_NUMBER :
			to_value->data.number = from_value->data.number;
		break;
		case A_JT_OBJECT : 
		{
			AJsonObject *from_object, *to_object, *previous = NULL;
			for(from_object = from_value->data.object; from_object != NULL; from_object = from_object->next)
			{
				uint i;
				to_object = malloc(sizeof * to_object);
				assemble_json_clone_internal(&from_object->value, &to_object->value);
				for(i = 0; from_object->name[i] != 0; i++);
				to_object->name = malloc((sizeof *from_object->name) * ++i);
				for(i = 0; from_object->name[i] != 0; i++)
					to_object->name[i] = from_object->name[i];
				to_object->name[i] = 0;
				if(previous == NULL)
					to_value->data.object = to_object;
				else
					previous->next = to_object;
				previous = to_object;
			}
			if(previous != NULL)
				previous->next = NULL;
		}
		break;
		case A_JT_ARRAY :
		{
			AJsonValueArray *from_array_value, *to_array_value, *previous = NULL;

	//		AJsonValueArray *v;
	//		for(v = value->data.array; v != NULL;)


			for(from_array_value = from_value->data.array; from_array_value != NULL; from_array_value = from_array_value->next)
			{
				to_array_value = malloc(sizeof *to_array_value);
				assemble_json_clone_internal(&from_array_value->value, &to_array_value->value);
				to_array_value->value.type &= ASSEMBLE_JSON_ARRAY;
				if(previous == NULL)
					to_value->data.array = to_array_value;
				else
					previous->next = to_array_value;
				previous = to_array_value;
			}
			if(previous != NULL)
				previous->next = NULL;
			else
				to_value->data.array = NULL;
		}
		break;
		case A_JT_TRUE :
		break;
		case A_JT_FALSE :
		break;
		case A_JT_NULL :
		break;
	}
}

AJsonValue *assemble_json_clone(AJsonValue *value)
{
	AJsonValue *to_value;
	to_value = malloc(sizeof *to_value);
	assemble_json_clone_internal(value, to_value);
	return to_value;
}


uint assemble_json_print_size(AJsonValue *value, uint indentation)
{
	uint i, pos = 0, linebreak;
	if(value == NULL)
		return 0;
	switch(value->type & ASSEMBLE_JSON_TYPE_MASK)
	{
		case A_JT_STRING :
			for(i = 0; value->data.string[i] != 0; i++);
		return i + 2;
		case A_JT_NUMBER :
		{
			char number[128];
	//		_snprintf(number, 128, "%f", value->data.number);
			sprintf(number, "%f", value->data.number);
			for(i = 0; number[i] != 0; i++);
			for(i--; i > 1 && number[i] == '0'; i--);
			if(number[i] == '.')
			{	
				if(i == 0)
					return 1;
				return i;
			}else
				return i + 1;
		}
		return pos;
		case A_JT_OBJECT : 
		{
			AJsonObject *object;
			pos += 2;
			for(object = value->data.object; object != NULL;)
			{
				pos += indentation + 1;
				for(i = 0; object->name[i] != 0; i++);
				pos += i;
				pos += 3;
				pos += assemble_json_print_size(&object->value, indentation + 1);
				object = object->next;
				if(object != NULL)
				{
					pos += 2;
				}else
					pos += 1;
			}
			pos += indentation;
			pos += 1;
		}
		return pos;
		case A_JT_ARRAY :
		{	
			AJsonValueArray *v;
			pos++;
			for(v = value->data.array; v != NULL && v->value.type != A_JT_STRING && v->value.type != A_JT_OBJECT && v->value.type != A_JT_ARRAY; v = v->next);
			linebreak = v != NULL;
			i = 0;
			for(v = value->data.array; v != NULL;)
			{			
				pos += assemble_json_print_size(&v->value, indentation);
				v = v->next;
				if(v != NULL)
				{
					pos += 2;
					if(linebreak || i % 16 == 15)
						pos += indentation;
				}
				i++;
			}
			pos++;
		}
		return pos;
		case A_JT_TRUE :
		return 4;
		case A_JT_FALSE :
		return 5;
		case A_JT_NULL :
		return 4;
	}	
	return 0;
}

uint assemble_json_print(char *array, AJsonValue *value, uint indentation)
{
	uint i, pos = 0, linebreak;
	if(value == NULL)
		return 0;
	switch(value->type & ASSEMBLE_JSON_TYPE_MASK)
	{
		case A_JT_STRING :
			array[pos++] = '\"';
			for(i = 0; value->data.string[i] != 0; i++)
				array[pos++] = value->data.string[i];
			array[pos++] = '\"';
		return pos;
		case A_JT_NUMBER :
		{
			char number[128];
	//		_snprintf(number, 128, "%f", value->data.number);
			sprintf(number, "%f", value->data.number);
			for(i = 0; number[i] != 0; i++);
			for(i--; i > 1 && number[i] == '0'; i--);
			if(number[i] == '.')
			{	
				if(i == 0)
				{
					array[0] = '0';
					return 1;
				}
				number[i] = 0;
			}else
				number[i + 1] = 0;
			for(i = 0; number[i] != 0; i++)
				array[pos++] = number[i];
		}
		return pos;
		case A_JT_OBJECT : 
		{
			AJsonObject *object;
			array[pos++] = '{';
			array[pos++] = '\n';
			for(object = value->data.object; object != NULL;)
			{
				for(i = 0; i < indentation + 1; i++)
					array[pos++] = '\t';
				for(i = 0; object->name[i] != 0; i++)
					array[pos++] = object->name[i];
				array[pos++] = ' ';
				array[pos++] = ':';
				array[pos++] = ' ';
				pos += assemble_json_print(&array[pos], &object->value, indentation + 1);
				object = object->next;
				if(object != NULL)
				{
					array[pos++] = ',';
					array[pos++] = '\n';
				}else
					array[pos++] = '\n';
			}
			if(i == 0)
				i = 0;
			for(i = 0; i < indentation; i++)
				array[pos++] = '\t';
			array[pos++] = '}';
		}
		return pos;
		case A_JT_ARRAY :
		{	
			AJsonValueArray *v;
			array[pos++] = '[';
			for(v = value->data.array; v != NULL && v->value.type != A_JT_STRING && v->value.type != A_JT_OBJECT && v->value.type != A_JT_ARRAY; v = v->next);
			linebreak = v != NULL;
			i = 0;
			for(v = value->data.array; v != NULL;)
			{			
				pos += assemble_json_print(&array[pos], &v->value, indentation);
				v = v->next;
				if(v != NULL)
				{
					if(linebreak || i % 16 == 15)
					{
						array[pos++] = ',';					
						array[pos++] = '\n';
						for(i = 0; i < indentation; i++)
							array[pos++] = '\t';
					}else
					{
						array[pos++] = ',';					
						array[pos++] = ' ';
					}
				}
				i++;
			}
			array[pos++] = ']';
		}
		return pos;
		case A_JT_TRUE :
			array[pos++] = 't';
			array[pos++] = 'r';
			array[pos++] = 'u';
			array[pos++] = 'e';
		return pos;
		case A_JT_FALSE :
			array[pos++] = 'f';
			array[pos++] = 'a';
			array[pos++] = 'l';
			array[pos++] = 's';
			array[pos++] = 'e';
		return pos;
		case A_JT_NULL :
			array[pos++] = 'n';
			array[pos++] = 'u';
			array[pos++] = 'l';
			array[pos++] = 'l';
		return pos;
	}	
	return 0;
}

char *assemble_json_print_allocate(AJsonValue *value)
{
	uint size;
	char *array;
	size = assemble_json_print_size(value, 0);
	array = malloc(size + 1);
	assemble_json_print(array, value, 0);
	array[size] = 0;
	return array;
}

AJsonValue *assemble_json_parse(char *string, int write_protected)
{
	uint8 *memory, *write;
	AJsonValue *output;
	uint size, size2;
	printf("Start");
	memory = (uint8 *)NULL;
	while(*string != '[' && *string != '{' && *string != 0)
		string++;
	if(*string == 0)
		return NULL;
	assemble_json_parse_value(NULL, string, &memory);
	if(memory == NULL)
		return NULL;
	memory += sizeof(AJsonValue);
	size = (size_t)memory;
	memory = malloc((size_t)memory); /*  Favorit line of code ever */
	write = &memory[sizeof(AJsonValue)];
	printf("Redo---------------------------------\n");
	assemble_json_parse_value((AJsonValue *)memory, string, &write);
	size2 = write - memory;
	if(write_protected)
		return (AJsonValue *)memory;
	{
		uint pos; 
		uint8 *array;
		pos = assemble_json_print_size(memory, 0);
		array = malloc(pos + 1);
		assemble_json_print(array, memory, 0);
		array[pos] = 0;
		printf("%s", array);
		free(array);	
	}

	output = assemble_json_clone((AJsonValue *)memory);
	free(memory);
	return output;
}

AJsonObject *assemble_json_object_member_get_next(AJsonObject *object)
{
	return object->next;
}
 
char *assemble_json_object_member_get_name(AJsonObject *object)
{
	return object->name;
} 

AJsonObject *assemble_json_object_member_search_name_get_object(AJsonObject *object, char *name, uint type)
{
	uint i;
	while(object != NULL)
	{
		for(i = 0; name[i] == object->name[i] && name[i] != 0; i++);
		if(name[i] == object->name[i] && (type == -1 || type == (object->value.type & ASSEMBLE_JSON_TYPE_MASK)))
			return object;
		object = object->next;
	}
	return NULL;
}

AJsonValue *assemble_json_object_member_search_name_get_value(AJsonObject *object, char *name, uint type)
{
	uint i;
	while(object != NULL)
	{
		for(i = 0; name[i] == object->name[i] && name[i] != 0; i++);
		if(name[i] == object->name[i])
		{
			if(type == -1 || type == (object->value.type & ASSEMBLE_JSON_TYPE_MASK))
				return &object->value;
			i = 0;
		}
		object = object->next;
	}
	return NULL;
} 

AJsonValue *assemble_json_object_member_get_value(AJsonObject *object)
{
	return &object->value;
}

AJsonType assemble_json_value_get_type(AJsonValue *value)
{
	return value->type & ASSEMBLE_JSON_TYPE_MASK;
}

char *assemble_json_value_get_string(AJsonValue *value)
{
	if(value->type & (ASSEMBLE_JSON_TYPE_MASK == A_JT_STRING))
		return value->data.string;
	printf("JSON: Error: Trying to call assemble_json_value_get_string on value not containg a string \n");
	return NULL;
}

double assemble_json_value_get_number_double(AJsonValue *value)
{
	if(value->type & (ASSEMBLE_JSON_TYPE_MASK == A_JT_NUMBER))
		return (double)value->data.number;
	printf("JSON: Error: Trying to call assemble_json_value_get_number on value not containg a string \n");
	return 0;
}

float assemble_json_value_get_number_float(AJsonValue *value)
{
	if(value->type & (ASSEMBLE_JSON_TYPE_MASK == A_JT_NUMBER))
		return (float)value->data.number;
	printf("JSON: Error: Trying to call assemble_json_value_get_number on value not containg a string \n");
	return 0;
}

AJsonObject *assemble_json_value_get_object(AJsonValue *value)
{
	if(value->type & (ASSEMBLE_JSON_TYPE_MASK == A_JT_OBJECT))
		return value->data.object;
	printf("JSON: Error: Trying to call assemble_json_value_get_string on value not containg a string \n");
	return NULL;
}

AJsonValue *assemble_json_value_get_array(AJsonValue *value)
{
	if(value->type & (ASSEMBLE_JSON_TYPE_MASK == A_JT_ARRAY))
		return value->data.array;
	printf("JSON: Error: Trying to call assemble_json_value_get_string on value not containg a string \n");
	return NULL;
}

AJsonValue *assemble_json_value_get_next(AJsonValue *value)
{
	if(value->type & ASSEMBLE_JSON_ARRAY)
		return ((AJsonValueArray *)value)->next;
	printf("JSON: Error: Trying to call assemble_json_value_get_next on a value not that is not part of an array \n");
	return NULL;
}
