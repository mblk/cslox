#ifndef _clox_table_h_
#define _clox_table_h_

#include "value.h"

#include <stddef.h>

typedef struct string_object string_object_t;

typedef struct {
    value_t key;
    value_t value;
} entry_t;

typedef struct {
    size_t capacity;
    size_t count;
    entry_t* entries;
} table_t;

void table_init(table_t* table);
void table_free(table_t* table);

bool table_get(const table_t* table, value_t key, value_t* value_out);
bool table_get_by_string(const table_t* table, const char* key, size_t length, const string_object_t** key_out, value_t* value_out); // compares key by content
bool table_set(table_t* table, value_t key, value_t value);
void table_add_all(table_t* target, const table_t* source);
bool table_delete(table_t* table, value_t key);

void table_dump(const table_t* table, const char* name);

#endif
