#include "object.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "hash.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_object(object_t* obj);

void object_root_init(object_root_t* root) {
    assert(root);

    root->first = NULL;

    table_init(&root->strings);
}

void object_root_free(object_root_t* root) {
    assert(root);

    object_t* current = root->first;
    while (current) {
        object_t* next = current->next;
        free_object(current);
        current = next;
    }

    root->first = NULL;

    table_free(&root->strings);
}

void object_root_dump(object_root_t* root, const char* name) {
    assert(root);

    printf("== object root '%s' ==\n", name);

    object_t* current = root->first;
    while (current) {
        printf("-> '");
        print_object(OBJECT_VALUE(current));
        printf("'\n");
        current = current->next;
    }

    table_dump(&root->strings, "strings");
}

static object_t* create_object(object_root_t* root, size_t size, object_type_t type) {
    assert(root);
    assert(type == OBJECT_TYPE_STRING);

    object_t* obj = ALLOC_BY_SIZE(object_t, size);
    assert(obj);

    memset(obj, 0, size);
    obj->type = type;

    // add to list
    obj->next = root->first;
    root->first = obj;

    return obj;
}

const string_object_t* create_string_object(object_root_t* root, const char* chars, size_t length) {
    assert(chars);
    // length=0 is legal (empty strings)

    // try to intern string (ie. reuse existing string objects with the same content)
    {
        const string_object_t* existing_obj = NULL;
        if (table_get_by_string(&root->strings, chars, length, &existing_obj, NULL)) {
            return existing_obj;
        }
    }

    // create new string object
    {
        string_object_t* obj = (string_object_t*)create_object(root, sizeof(string_object_t) + length + 1, OBJECT_TYPE_STRING);
        assert(obj);
        
        obj->hash = hash_string(chars, length);
        obj->length = length;
        memcpy(obj->chars, chars, length);
        obj->chars[length] = '\0';
    
        table_set(&root->strings, OBJECT_VALUE((object_t*)obj), NIL_VALUE());
        
        return obj;
    }
}

static void free_object(object_t* obj) {
    assert(obj);

    switch (obj->type) {
        case OBJECT_TYPE_STRING: {
            string_object_t *string = (string_object_t*)obj;
            FREE_BY_SIZE(string, sizeof(string_object_t) + string->length + 1);
            break;
        }

        default:
            assert(!"Missing case in free_object");
            break;
    }
}

bool objects_equal(value_t a, value_t b) {
    assert(IS_OBJECT(a));
    assert(IS_OBJECT(b));

    if (IS_STRING(a) && IS_STRING(b)) {
        const string_object_t* string_a = AS_STRING(a);
        const string_object_t* string_b = AS_STRING(b);

        return string_a == string_b; // Note: Comparing by address. This is possible due to string-interning.

        // Without string interning:
        // if (string_a->length == string_b->length &&
        //     memcmp(string_a->chars, string_b->chars, string_a->length) == 0) {
        //     return true;
        // }
    }

    return false;
}

void print_object(value_t value) {
    assert(IS_OBJECT(value));

    switch (OBJECT_TYPE(value)) {
        case OBJECT_TYPE_STRING: {
            //const string_object_t* string = AS_STRING(value);
            printf("%s", AS_C_STRING(value));
            //printf(" hash=%u", string->hash);
            break;
        }

        default:
            assert(!"Missing case in print_object");
            break;
    }
}

void print_object_to_buffer(char* buffer, size_t max_length, value_t value) {
    assert(IS_OBJECT(value));

    switch (OBJECT_TYPE(value)) {
        case OBJECT_TYPE_STRING: {
            //const string_object_t* string = AS_STRING(value);
            snprintf(buffer, max_length, "%s", AS_C_STRING(value));
            //printf(" hash=%u", string->hash);
            break;
        }

        default:
            assert(!"Missing case in print_object_to_buffer");
            snprintf(buffer, max_length, "???");
            break;
    }
}

uint32_t hash_object(value_t value) {
    assert(IS_OBJECT(value));

    switch (OBJECT_TYPE(value)) {
        case OBJECT_TYPE_STRING: {
            const string_object_t* string = AS_STRING(value);
            return string->hash;
        }

        // TODO for later:
        // maybe use GetHashCode()/Equals() approach from .NET so any user-defined object can be used as key in a hashmap?

        default:
            assert(!"Missing case in hash_object");
            return 0;
    }
}