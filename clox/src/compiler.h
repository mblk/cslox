#ifndef _clox_compiler_h_
#define _clox_compiler_h_

typedef struct object_root object_root_t;
typedef struct chunk chunk_t;

bool compile(object_root_t* root, chunk_t* chunk, const char* source);

#endif
