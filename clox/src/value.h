#ifndef _clox_value_h_
#define _clox_value_h_

#include <stddef.h>
#include <stdint.h>

typedef struct object object_t;

typedef enum {
    VALUE_TYPE_NIL,
    VALUE_TYPE_BOOL,    // stored in value_t
    VALUE_TYPE_NUMBER,  // stored in value_t
    VALUE_TYPE_OBJECT,  // stored on heap
} value_type_t;

typedef struct {
    value_type_t type;
    union {
        bool boolean;
        double number;
        object_t* object;
    } as;
    // TODO maybe precompute hash for all values ?
} value_t;

// 64bits for type
// 64bits for value
static_assert(sizeof(value_t) == 16);

// c-type to lox-type
#define NIL_VALUE()         ((value_t){VALUE_TYPE_NIL,    {.number = 0}})
#define BOOL_VALUE(value)   ((value_t){VALUE_TYPE_BOOL,   {.boolean = value}})
#define NUMBER_VALUE(value) ((value_t){VALUE_TYPE_NUMBER, {.number = value}})
#define OBJECT_VALUE(value) ((value_t){VALUE_TYPE_OBJECT, {.object = value}})

// lox-type to c-type
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)
#define AS_OBJECT(value)    ((value).as.object)

// type checking
#define IS_NIL(value)       ((value).type == VALUE_TYPE_NIL)
#define IS_BOOL(value)      ((value).type == VALUE_TYPE_BOOL)
#define IS_NUMBER(value)    ((value).type == VALUE_TYPE_NUMBER)
#define IS_OBJECT(value)    ((value).type == VALUE_TYPE_OBJECT)

typedef struct value_array {
    size_t capacity;
    size_t count;
    value_t *values;
} value_array_t;

void value_array_init(value_array_t* array);
void value_array_free(value_array_t* array);

size_t value_array_write(value_array_t* array, value_t value);
void value_array_dump(const value_array_t* array);

void print_value(value_t value);
void print_value_to_buffer(char* buffer, size_t max_length, value_t value);
uint32_t hash_value(value_t value);

bool value_is_truey(value_t value);
bool value_is_falsey(value_t value);
bool values_equal(value_t a, value_t b);

#endif
