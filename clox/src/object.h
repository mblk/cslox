#ifndef _clox_object_h_
#define _clox_object_h_

#include "value.h"
#include "table.h"
#include "chunk.h"

#include <stdint.h>

typedef enum {
    OBJECT_TYPE_STRING,
    OBJECT_TYPE_FUNCTION,
    OBJECT_TYPE_NATIVE,
} object_type_t;

typedef struct object {
    object_type_t type;
    struct object* next; // single linked list for all objects
} object_t;

typedef struct object_root {
    object_t* first; // single linked list for all objects
    table_t strings;
} object_root_t;

typedef struct string_object {
    object_t object;
    uint32_t hash;
    size_t length;
    char chars[]; // "Flexible array member"
} string_object_t;

typedef struct function_object {
    object_t object;
    const string_object_t* name;
    size_t arity;
    chunk_t chunk;
} function_object_t;

typedef value_t (*native_fn_t)(void* context, size_t arg_count, const value_t* args);

typedef struct native_object {
    object_t object;
    const char* name; // ptr to rodata
    size_t arity;
    native_fn_t fn;
} native_object_t;

// struct inheritance:
// object_t:            [type] [next]
// string_object_t:     [type] [next] [hash] [length] [chars...]
// function_object_t:   [type] [next] [name] [arity] [chunk]
// native_object_t:     [type] [next] [name] [arity] [fn]
// ...

// value to object
#define OBJECT_TYPE(value)      (AS_OBJECT(value)->type)

// type checking
#define IS_STRING(value)        is_object_type(value, OBJECT_TYPE_STRING)
#define IS_FUNCTION(value)      is_object_type(value, OBJECT_TYPE_FUNCTION)
#define IS_NATIVE(value)        is_object_type(value, OBJECT_TYPE_NATIVE)

// value to c-type
#define AS_STRING(value)        ((string_object_t*)AS_OBJECT(value))
#define AS_C_STRING(value)      (((string_object_t*)AS_OBJECT(value))->chars)
#define AS_FUNCTION(value)      ((function_object_t*)AS_OBJECT(value))
#define AS_NATIVE(value)        ((native_object_t*)AS_OBJECT(value))

// Function instead of macro to prevent double-evaluation
[[maybe_unused]]
static inline bool is_object_type(value_t value, object_type_t object_type) {
    return IS_OBJECT(value) && OBJECT_TYPE(value) == object_type;
}

void object_root_init(object_root_t* root);
void object_root_free(object_root_t* root);
void object_root_dump(object_root_t* root, const char* name);

const string_object_t* create_string_object(object_root_t* root, const char* chars, size_t length);
function_object_t* create_function_object(object_root_t* root);
native_object_t* create_native_object(object_root_t* root, const char* name, size_t arity, native_fn_t fn);

uint32_t hash_object(value_t value);
bool objects_equal(value_t a, value_t b);

void print_object(value_t value);
void print_object_to_buffer(char* buffer, size_t max_length, value_t value);

#endif
