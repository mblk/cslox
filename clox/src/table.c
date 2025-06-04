#include "table.h"
#include "value.h"
#include "object.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

// Notes:
// - Using open addressing with linear probing.
// - Deleted entries are marked as "tombstone" (key==null, value=true) to keep the probing sequence intact

void table_init(table_t* table) {
    assert(table);

    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

void table_free(table_t* table) {
    assert(table);

    FREE_BY_COUNT(entry_t, table->entries, table->capacity);

    table_init(table);
}

static entry_t *find_entry(const table_t* table, const string_object_t* key) {
    assert(table->capacity > 0);

    size_t index = key->hash % table->capacity;
    entry_t* tombstone = NULL;

    for (;;) {
        entry_t* const entry = table->entries + index;

        // Bucket empty or tombstone?
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) { // Found empty bucket
                // Reuse first tombstones we found on the way here.
                // If none were found, return the empty entry instead.
                return tombstone ? tombstone : entry;
            } else { // Found tombstone
                // Remember the first tombstone we find.
                if (tombstone == NULL) tombstone = entry;
            }
        }
        // Is this the key we are looking for?
        else if (entry->key == key) { // Note: comparing keys by address.
            return entry;
        }

        // open addressing + linear probing
        index = (index + 1) % table->capacity;
    }

    assert(!"Must not reach");
    return NULL;
}

static entry_t *find_entry_by_string(const table_t* table, const char* key, size_t length) {
    assert(table->capacity > 0);

    const uint32_t hash = hash_string(key, length);

    size_t index = hash % table->capacity;
    entry_t* tombstone = NULL;

    for (;;) {
        entry_t* const entry = table->entries + index;

        // Bucket empty or tombstone?
        if (entry->key == NULL) {
            if (IS_NIL(entry->value)) { // Found empty bucket
                // Reuse first tombstones we found on the way here.
                // If none were found, return the empty entry instead.
                return tombstone ? tombstone : entry;
            } else { // Found tombstone
                // Remember the first tombstone we find.
                if (tombstone == NULL) tombstone = entry;
            }
        }
        // Is this the key we are looking for?
        else if (entry->key->hash == hash &&
                 entry->key->length == length &&
                 memcmp(entry->key->chars, key, length) == 0) { // Note: comparing keys by content.
            return entry;
        }

        // open addressing + linear probing
        index = (index + 1) % table->capacity;
    }

    assert(!"Must not reach");
    return NULL;
}

static void adjust_capacity(table_t* table, size_t new_capacity) {
    entry_t *const old_entries = table->entries;
    const size_t old_capacity = table->capacity;

    // create new entries
    entry_t* new_entries = ALLOC_BY_COUNT(entry_t, new_capacity);
    assert(new_entries);

    for (size_t i=0; i<new_capacity; i++) {
        new_entries[i].key = NULL;
        new_entries[i].value = NIL_VALUE();
    }

    table->capacity = new_capacity;
    table->entries = new_entries;

    // rebuild old entries
    size_t new_count = 0;
    for (size_t i=0; i<old_capacity; i++) {
        const entry_t* src = old_entries + i;
        if (!src->key) continue;

        entry_t* dest = find_entry(table, src->key);
        dest->key = src->key;
        dest->value = src->value;
        new_count++;
    }

    table->count = new_count;

    FREE_BY_COUNT(entry_t, old_entries, old_capacity);
}

bool table_get(const table_t* table, const string_object_t* key, value_t* value) {
    assert(table);
    assert(key);

    if (table->count == 0) {
        return false;
    }

    const entry_t* entry = find_entry(table, key);
    assert(entry);

    if (!entry->key) {
        return false;
    }
    
    // Optionally return the value.
    if (value) {
        *value = entry->value;
    }

    return true;
}

bool table_get_by_string(const table_t* table, const char* key, size_t length, const string_object_t** key_out, value_t* value_out) {
    assert(table);
    assert(key);

    if (table->count == 0) {
        return false;
    }

    const entry_t* entry = find_entry_by_string(table, key, length);
    assert(entry);

    if (!entry->key) {
        return false;
    }
    
    // Optionally return the key object.
    if (key_out) {
        *key_out = entry->key;
    }

    // Optionally return the value.
    if (value_out) {
        *value_out = entry->value;
    }

    return true;
}

bool table_set(table_t* table, const string_object_t* key, value_t value) {
    assert(table);
    assert(key);

    // must grow?
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        const size_t new_cap = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, new_cap);
        assert(table->capacity == new_cap);
        assert(table->entries);
    }

    entry_t* entry = find_entry(table, key);
    assert(entry);

    const bool is_new = entry->key == NULL;
    const bool is_tombstone = entry->key == NULL && !IS_NIL(entry->value);

    if (is_new && !is_tombstone) {
        table->count++; // Count is the number of active entries + the number of tombstones
    }

    entry->key = key;
    entry->value = value;

    return is_new;
}

void table_add_all(table_t* target, const table_t* source) {
    assert(target);
    assert(source);

    for (size_t i=0; i<source->capacity; i++) {
        const entry_t* source_entry = source->entries + i;
        if (!source_entry->key) continue;

        table_set(target, source_entry->key, source_entry->value);
    }
}

bool table_delete(table_t* table, const string_object_t* key) {
    assert(table);
    assert(key);

    if (table->count == 0) {
        return false;
    }

    entry_t* entry = find_entry(table, key);
    if (entry->key == NULL) {
        return false;
    }

    entry->key = NULL;
    entry->value = BOOL_VALUE(true); // mark entry as a "tombstone" to keep the probe-sequence intact

    return true;
}

static void table_check_duplicates(const table_t* table) {
    table_t temp;
    table_init(&temp);

    for (size_t i=0; i<table->capacity; i++) {
        const entry_t* entry = table->entries + i;
        if (!entry->key) continue;

        if (table_get_by_string(&temp, entry->key->chars, entry->key->length, NULL, NULL)) {
            printf("Error: Table has duplicate key: '%.*s'\n", (int)entry->key->length, entry->key->chars);
        }

        table_set(&temp, entry->key, NIL_VALUE());
    }

    table_free(&temp);
}

void table_dump(const table_t* table, const char* name) {
    assert(table);

    printf("=== table '%s' ===\n", name);
    printf("count/capacity: %zu/%zu\n", table->count, table->capacity);
    printf("[i] key                     -> value\n");
    printf("====================================\n");

    size_t count = 0;
    for (size_t i=0; i<table->capacity; i++) {
        const entry_t* const entry = table->entries + i;

        const char* key = entry->key ? entry->key->chars : "---";
        const size_t key_len = entry->key ? entry->key->length : 3;
        const char* quotes = entry->key ? "'" : " ";

        printf("[%zu] %s%s%s %-*s -> ", i, quotes, key, quotes, (int)(20-key_len), "");
        print_value(entry->value);
        printf("\n");

        if (entry->key != NULL || (entry->key == NULL && IS_BOOL(entry->value))) {
            count++; // Number of active entries + number of tombstones
        }
    }

    if (count != table->count) {
        printf("Error: Table has incorrect count (is %zu, should be %zu)\n", table->count, count);
    }

    table_check_duplicates(table);
}
