#include "value.h"
#include "memory.h"
#include "object.h"
#include "hash.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

void value_array_init(value_array_t* array) {
    assert(array);

    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void value_array_free(value_array_t* array) {
    assert(array);

    array->values = GROW_ARRAY(value_t, array->values, array->capacity, 0);
    array->capacity = 0;
    array->count = 0;
}

size_t value_array_write(value_array_t* array, value_t value) {
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

void value_array_dump(const value_array_t* array) {
    assert(array);

    printf("== value array (%zu / %zu) ==\n", array->count, array->capacity);

    for (size_t i=0; i<array->count; i++) {
        printf("[%zu] = ", i);
        print_value(array->values[i]);
        printf("\n");
    }
}

uint32_t hash_value(value_t value) {
    assert(value.type == VALUE_TYPE_NIL ||
           value.type == VALUE_TYPE_BOOL ||
           value.type == VALUE_TYPE_NUMBER ||
           value.type == VALUE_TYPE_OBJECT);

    switch (value.type) {
        case VALUE_TYPE_NIL: return hash_nil();
        case VALUE_TYPE_BOOL: return hash_bool(AS_BOOL(value));
        case VALUE_TYPE_NUMBER: return hash_double(AS_NUMBER(value));
        case VALUE_TYPE_OBJECT: return hash_object(value);

        default:
            assert(!"Missing case in hash_value()");
            return 0;
    }
}

void print_value(value_t value) {
    assert(value.type == VALUE_TYPE_NIL ||
           value.type == VALUE_TYPE_BOOL ||
           value.type == VALUE_TYPE_NUMBER ||
           value.type == VALUE_TYPE_OBJECT);
    
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
        case VALUE_TYPE_OBJECT:
            print_object(value);
            break;
        default:
            assert(!"Missing case in print_value()");
            printf("???");
            break;
    }
}

void print_value_to_buffer(char* buffer, size_t max_length, value_t value) {
    assert(buffer);
    assert(max_length);

    assert(value.type == VALUE_TYPE_NIL ||
           value.type == VALUE_TYPE_BOOL ||
           value.type == VALUE_TYPE_NUMBER ||
           value.type == VALUE_TYPE_OBJECT);
    
    switch (value.type) {
        case VALUE_TYPE_NIL:
            snprintf(buffer, max_length, "nil");
            break;
        case VALUE_TYPE_BOOL:
            snprintf(buffer, max_length, "%s", AS_BOOL(value) ? "true" : "false");
            break;
        case VALUE_TYPE_NUMBER:
            snprintf(buffer, max_length, "%lf", AS_NUMBER(value));
            break;
        case VALUE_TYPE_OBJECT:
            print_object_to_buffer(buffer, max_length, value);
            break;
        default:
            assert(!"Missing case in print_value()");
            snprintf(buffer, max_length, "???");
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

bool values_equal(value_t a, value_t b) {
    if (a.type != b.type) {
        return false;
    }

    switch (a.type) {
        case VALUE_TYPE_NIL:
            return true;

        case VALUE_TYPE_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);

        case VALUE_TYPE_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);

        case VALUE_TYPE_OBJECT:
            return objects_equal(a, b);

        default:
            assert(!"Missing case in values_equal");
            return false;
    }
}
