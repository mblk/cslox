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

size_t value_array_write(value_array_t* array, value_t value)
{
    assert(array);

    if (array->count + 1 > array->capacity) {
        const size_t old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(value_t, array->values, old_capacity, array->capacity);
    }

    const size_t insert_index = array->count;

    array->values[insert_index] = value;
    array->count++;

    return insert_index;
}

void value_array_dump(const value_array_t* array)
{
    assert(array);

    printf("== value array (%zu / %zu) ==\n", array->count, array->capacity);

    for (size_t i=0; i<array->count; i++) {
        printf("[%zu] = ", i);
        print_value(array->values[i]);
        printf("\n");
    }
}

void print_value(value_t value)
{
    assert(value.type == VALUE_TYPE_NIL ||
           value.type == VALUE_TYPE_BOOL ||
           value.type == VALUE_TYPE_NUMBER);
    
    switch (value.type) {
        case VALUE_TYPE_NIL:
            printf("nil");
            break;
        case VALUE_TYPE_BOOL:
            printf("%s", AS_BOOL(value) ? "true" : "false");
            break;
        case VALUE_TYPE_NUMBER:
            printf("%lf", AS_NUMBER(value));
            break;
    }
}

bool value_is_truey(value_t value) {
    return !value_is_falsey(value);
}

// Note: Function instead of macro to prevent double-evaluation
bool value_is_falsey(value_t value) {
    if (IS_NIL(value)) {
        return true;
    }
    if (IS_BOOL(value) && AS_BOOL(value) == false) {
        return true;
    }
    return false;
}
