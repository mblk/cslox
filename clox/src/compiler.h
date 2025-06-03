#ifndef _clox_compiler_h_
#define _clox_compiler_h_

typedef struct chunk chunk_t;

bool compile(chunk_t* chunk, const char* source);

#endif
