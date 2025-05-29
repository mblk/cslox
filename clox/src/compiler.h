#ifndef _clox_compiler_h_
#define _clox_compiler_h_

#include <stdbool.h>

typedef struct chunk chunk_t;

bool compile(chunk_t* chunk, const char* source);

#endif
