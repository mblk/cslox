#ifndef _clox_value_h_
#define _clox_value_h_

#include "common.h"

typedef double value_t;

typedef struct value_array {
    size_t capacity;
    size_t count;
    value_t *values;
} value_array_t;

void value_array_init(value_array_t* array);
void value_array_free(value_array_t* array);

void value_array_write(value_array_t* array, value_t value);
void value_array_dump(const value_array_t* array);

#endif
