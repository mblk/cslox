#ifndef _clox_debug_h_
#define _clox_debug_h_

#include <stddef.h>

typedef struct chunk chunk_t;

void disassemble_chunk(const chunk_t* chunk, const char *name);
size_t disassemble_instruction(const chunk_t* chunk, size_t offset);

#endif
