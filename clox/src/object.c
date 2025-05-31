#include "object.h"
#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

string_object_t* create_emppy_string_object(object_root_t* root, size_t length) {
    // length=0 is legal case: for empty strings
    
    string_object_t* obj = (string_object_t*)create_object(root, sizeof(string_object_t) + length + 1, OBJECT_TYPE_STRING);
    assert(obj);
    
    obj->length = length;
    memset(obj->chars, 0, length + 1);
    
    return obj;
}

string_object_t* create_string_object(object_root_t* root, const char* chars, size_t length) {
    assert(chars);
    // length=0 is legal case: for empty strings

    string_object_t* obj = create_emppy_string_object(root, length);
    assert(obj);
        
    memcpy(obj->chars, chars, length); // terminating 0 already set
    
    return obj;
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

void free_objects(object_root_t* root) {
    assert(root);

    object_t* current = root->first;
    while (current) {
        object_t* next = current->next;
        free_object(current);
        current = next;
    }

    root->first = NULL;
}

bool objects_equal(value_t a, value_t b) {
    assert(IS_OBJECT(a));
    assert(IS_OBJECT(b));

    if (IS_STRING(a) && IS_STRING(b)) {
        const string_object_t* string_a = AS_STRING(a);
        const string_object_t* string_b = AS_STRING(b);

        if (string_a->length == string_b->length &&
            memcmp(string_a->chars, string_b->chars, string_a->length) == 0) {
            return true;
        }
    }

    return false;
}

void print_object(value_t value) {
    assert(IS_OBJECT(value));

    switch (OBJECT_TYPE(value)) {
        case OBJECT_TYPE_STRING:
            printf("%s", AS_C_STRING(value));
            break;

        default:
            assert(!"Missing case in print_object");
            break;
    }
}

void dump_objects(object_root_t* root) {
    //
    assert(root);

    printf("== object root ==\n");

    object_t* current = root->first;
    while (current) {
        printf("-> '");
        print_object(OBJECT_VALUE(current));
        printf("'\n");
        current = current->next;
    }
}
