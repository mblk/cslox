#ifndef _clox_value_h_
#define _clox_value_h_

#include "common.h"

typedef enum {
    VALUE_TYPE_NIL,
    VALUE_TYPE_BOOL,
    VALUE_TYPE_NUMBER,
} value_type_t;

typedef struct {
    value_type_t type;
    union {
        bool boolean;
        double number;
    } as;
} value_t;

// c-type to lox-type
#define NIL_VALUE()         ((value_t){VALUE_TYPE_NIL,    {.number = 0}})
#define BOOL_VALUE(value)   ((value_t){VALUE_TYPE_BOOL,   {.boolean = value}})
#define NUMBER_VALUE(value) ((value_t){VALUE_TYPE_NUMBER, {.number = value}})

// lox-type to c-type
#define AS_BOOL(value)      ((value).as.boolean)
#define AS_NUMBER(value)    ((value).as.number)

// type checking
#define IS_NIL(value)       ((value).type == VALUE_TYPE_NIL)
#define IS_BOOL(value)      ((value).type == VALUE_TYPE_BOOL)
#define IS_NUMBER(value)    ((value).type == VALUE_TYPE_NUMBER)


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

bool value_is_truey(value_t value);
bool value_is_falsey(value_t value);

#endif
