#ifndef _clox_compiler_h_
#define _clox_compiler_h_

typedef struct object_root object_root_t;
typedef struct function_object function_object_t;

const function_object_t* compile(object_root_t* root, const char* source);

#endif
