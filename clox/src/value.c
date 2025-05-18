#include "value.h"
#include "memory.h"

#include <stdio.h>
#include <assert.h>

void value_array_init(value_array_t* array)
{
    assert(array);

    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void value_array_free(value_array_t* array)
{
    assert(array);

    array->values = GROW_ARRAY(value_t, array->values, array->capacity, 0);
    array->capacity = 0;
    array->count = 0;
}

void value_array_write(value_array_t* array, value_t value)
{
    assert(array);

    if (array->count + 1 > array->capacity) {
        const size_t old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(value_t, array->values, old_capacity, array->capacity);
    }

    array->values[array->count++] = value;
}

void value_array_dump(const value_array_t* array)
{
    assert(array);

    printf("== value array (%zu / %zu) ==\n", array->count, array->capacity);

    for (size_t i=0; i<array->count; i++) {
        printf("[%zu] = %lf\n", i, array->values[i]);
    }
}