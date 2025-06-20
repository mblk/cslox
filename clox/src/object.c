#include "object.h"
#include "memory.h"
#include "table.h"
#include "value.h"
#include "hash.h"
#include "chunk.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_object(object_t* obj);

void object_root_init(object_root_t* root) {
    assert(root);

    root->first = NULL;
    root->open_upvalues = NULL;

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
    assert(type == OBJECT_TYPE_STRING ||
           type == OBJECT_TYPE_NATIVE ||
           type == OBJECT_TYPE_FUNCTION ||
           type == OBJECT_TYPE_CLOSURE ||
           type == OBJECT_TYPE_UPVALUE);

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
    assert(root);
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

native_object_t* create_native_object(object_root_t* root, const char* name, size_t arity, native_fn_t fn) {
    assert(root);
    assert(fn);
    
    native_object_t* obj = (native_object_t*)create_object(root, sizeof(native_object_t), OBJECT_TYPE_NATIVE);
    assert(obj);
    
    obj->name = name; // ptr to rodata
    obj->arity = arity;
    obj->fn = fn;
    
    return obj;
}

function_object_t* create_function_object(object_root_t* root) {
    assert(root);
    
    function_object_t* obj = (function_object_t*)create_object(root, sizeof(function_object_t), OBJECT_TYPE_FUNCTION);
    assert(obj);

    obj->name = NULL;
    obj->arity = 0;
    chunk_init(&obj->chunk);

    return obj;
}

closure_object_t* create_closure_object(object_root_t* root, const function_object_t* function) {
    assert(root);
    assert(function);

    // allocate before obj-alloc to prevent GC from seeing it.
    upvalue_object_t** upvalues = NULL;
    if (function->upvalue_count > 0) {
        upvalues = ALLOC_BY_COUNT(upvalue_object_t*, function->upvalue_count);
        assert(upvalues);
        memset(upvalues, 0, sizeof(upvalue_object_t*) * function->upvalue_count);

    }

    closure_object_t* obj = (closure_object_t*)create_object(root, sizeof(closure_object_t), OBJECT_TYPE_CLOSURE);
    assert(obj);

    obj->function = function;
    obj->upvalues = upvalues;
    obj->upvalue_count = function->upvalue_count;

    return obj;
}

upvalue_object_t* create_upvalue_object(object_root_t* root, value_t* target) {
    assert(root);
    assert(target);

    upvalue_object_t* obj = (upvalue_object_t*)create_object(root, sizeof(upvalue_object_t), OBJECT_TYPE_UPVALUE);
    assert(obj);

    obj->target = target;
    obj->closed = NIL_VALUE();
    obj->next = NULL;

    return obj;
}

static void free_object(object_t* obj) {
    assert(obj);

    switch (obj->type) {
        case OBJECT_TYPE_STRING: {
            string_object_t* const string = (string_object_t*)obj;
            FREE_BY_SIZE(string, sizeof(string_object_t) + string->length + 1);
            break;
        }
        
        case OBJECT_TYPE_NATIVE: {
            native_object_t* const native = (native_object_t*)obj;
            FREE_BY_COUNT(native_object_t, native, 1);
            break;
        }

        case OBJECT_TYPE_FUNCTION: {
            function_object_t* const function = (function_object_t*)obj;
            chunk_free(&function->chunk);
            // Note: function->name has independant lifetime
            FREE_BY_COUNT(function_object_t, function, 1);
            break;
        }
        
        case OBJECT_TYPE_CLOSURE: {
            closure_object_t* const closure = (closure_object_t*)obj;
            if (closure->upvalues) {
                FREE_BY_COUNT(upvalue_object_t*, closure->upvalues, closure->upvalue_count);
            }
            FREE_BY_COUNT(closure_object_t, closure, 1);
            break;
        }

        case OBJECT_TYPE_UPVALUE: {
            upvalue_object_t* const upvalue = (upvalue_object_t*)obj;
            // ...
            FREE_BY_COUNT(upvalue_object_t, upvalue, 1);
            break;
        }

        default: {
            assert(!"Missing case in free_object");
            break;
        }
    }
}

uint32_t hash_object(value_t value) {
    assert(IS_OBJECT(value));

    switch (OBJECT_TYPE(value)) {
        case OBJECT_TYPE_STRING: {
            const string_object_t* const string = AS_STRING(value);
            return string->hash;
        }

        case OBJECT_TYPE_NATIVE: {
            const native_object_t* const native = AS_NATIVE(value);
            const uint64_t addr = (uint64_t)native->fn;
            return (uint32_t)addr;
        }

        case OBJECT_TYPE_FUNCTION: {
            const function_object_t* const function = AS_FUNCTION(value);
            return function->name->hash;
        }

        case OBJECT_TYPE_CLOSURE: {
            const closure_object_t* const closure = AS_CLOSURE(value);
            return closure->function->name->hash;
        }

        case OBJECT_TYPE_UPVALUE: {
            return 123;
        }

        // TODO for later:
        // maybe use GetHashCode()/Equals() approach from .NET so any user-defined object can be used as key in a hashmap?

        default: {
            assert(!"Missing case in hash_object");
            return 0;
        }
    }
}

bool objects_equal(value_t a, value_t b) {
    assert(IS_OBJECT(a));
    assert(IS_OBJECT(b));

    if (IS_OBJECT(a) && IS_OBJECT(b)) {
        const object_t* const object_a = AS_OBJECT(a);
        const object_t* const object_b = AS_OBJECT(b);
    
        return object_a == object_b;
    }
    
    #if 0
    if (IS_STRING(a) && IS_STRING(b)) {
        const string_object_t* const string_a = AS_STRING(a);
        const string_object_t* const string_b = AS_STRING(b);

        return string_a == string_b; // Note: Comparing by address. This is possible due to string-interning.

        // Without string interning:
        // if (string_a->length == string_b->length &&
        //     memcmp(string_a->chars, string_b->chars, string_a->length) == 0) {
        //     return true;
        // }
    }

    if (IS_FUNCTION(a) && IS_FUNCTION(b)) {
        const function_object_t* const function_a = AS_FUNCTION(a);
        const function_object_t* const function_b = AS_FUNCTION(b);

        return function_a == function_b;
    }
    #endif

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
        
        case OBJECT_TYPE_NATIVE: {
            const native_object_t* const native = AS_NATIVE(value);
            if (native->name) {
                printf("<native fn %s>", native->name);
            } else {
                printf("<native fn>");
            }
            break;
        }

        case OBJECT_TYPE_FUNCTION: {
            const function_object_t* const function = AS_FUNCTION(value);
            if (function->name) {
                printf("<fn %s>", function->name->chars);
            } else {
                printf("<script>");
            }
            break;
        }

        case OBJECT_TYPE_CLOSURE: {
            const closure_object_t* const closure = AS_CLOSURE(value);
            if (closure->function->name) {
                printf("<fn %s>", closure->function->name->chars);
                //printf("<closure %s>", closure->function->name->chars);
            } else {
                printf("<script>");
                //printf("<script closure>");
            }
            break;
        }

        case OBJECT_TYPE_UPVALUE: {
            printf("upvalue");
            break;
        }

        default: {
            assert(!"Missing case in print_object");
            break;
        }
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

        case OBJECT_TYPE_NATIVE: {
            const native_object_t* const native = AS_NATIVE(value);
            if (native->name) {
                snprintf(buffer, max_length, "<native fn %s>", native->name);
            } else {
                snprintf(buffer, max_length, "<native fn>");
            }
            break;
        }

        case OBJECT_TYPE_FUNCTION: {
            const function_object_t* const function = AS_FUNCTION(value);
            if (function->name) {
                snprintf(buffer, max_length, "<fn %s>", function->name->chars);
            } else {
                snprintf(buffer, max_length, "<script>");
            }
            break;
        }

        case OBJECT_TYPE_CLOSURE: {
            const closure_object_t* const closure = AS_CLOSURE(value);
            if (closure->function->name) {
                snprintf(buffer, max_length, "<fn %s>", closure->function->name->chars);
                //snprintf(buffer, max_length, "<closure %s>", closure->function->name->chars);
            } else {
                snprintf(buffer, max_length, "<script>");
                //snprintf(buffer, max_length, "<script closure>");
            }
            break;
        }

        case OBJECT_TYPE_UPVALUE: {
            snprintf(buffer, max_length, "upvalue");
            break;
        }
        
        default: {
            assert(!"Missing case in print_object_to_buffer");
            snprintf(buffer, max_length, "???");
            break;
        }
    }
}
