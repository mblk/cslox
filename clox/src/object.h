#ifndef _clox_object_h_
#define _clox_object_h_

#include "value.h"

#include <stdint.h>

typedef enum {
    OBJECT_TYPE_STRING,
} object_type_t;

typedef struct object {
    object_type_t type;
    struct object* next; // single linked list for all objects
} object_t;

typedef struct {
    object_t* first; // single linked list for all objects
} object_root_t;

typedef struct string_object {
    object_t object;
    uint32_t hash;
    size_t length;
    char chars[]; // "Flexible array member"
} string_object_t;

// struct inheritance:
// object_t:            [type]
// string_object_t:     [type] [length] [chars...]

// value to object
#define OBJECT_TYPE(value)      (AS_OBJECT(value)->type)

// type checking
#define IS_STRING(value)        is_object_type(value, OBJECT_TYPE_STRING)

// value to c-type
#define AS_STRING(value)        ((string_object_t*)AS_OBJECT(value))
#define AS_C_STRING(value)      (((string_object_t*)AS_OBJECT(value))->chars)

// Function instead of macro to prevent double-evaluation
static inline bool is_object_type(value_t value, object_type_t object_type) {
    return IS_OBJECT(value) && OBJECT_TYPE(value) == object_type;
}

string_object_t* create_empty_string_object(object_root_t* root, size_t length, uint32_t hash);
string_object_t* create_string_object(object_root_t* root, const char* chars, size_t length);

void free_objects(object_root_t* root);

bool objects_equal(value_t a, value_t b);

void print_object(value_t value);
void dump_objects(object_root_t* root);

uint32_t hash_string(const char* start, size_t length);

#endif
