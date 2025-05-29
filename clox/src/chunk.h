#ifndef _clox_chunk_h_
#define _clox_chunk_h_

#include "common.h"
#include "value.h"

typedef enum {      // args:
    OP_INVALID,

    OP_CONST,       // index
    OP_CONST_LONG,  // index0, index1, index2, index3

    OP_NIL,         // -
    OP_TRUE,        // -
    OP_FALSE,       // -

    OP_NOT,         // -
    OP_NEGATE,      // -

    // next:
    //OP_EQUAL,       // -
    //OP_GREATER,     // -
    //OP_LESS,        // -

    OP_ADD,         // -
    OP_SUB,         // -
    OP_MUL,         // -
    OP_DIV,         // -

    OP_RETURN,      // -

} OpCode;

typedef struct {
    uint32_t line;  // which line
    uint32_t bytes; // how many following bytes in the instruction stream are at that line
} line_info_t;

typedef struct chunk {
    size_t capacity;
    size_t count;
    uint8_t *code;

    size_t line_infos_capacity;
    size_t line_infos_count;
    line_info_t *line_infos;

    value_array_t values;
} chunk_t;

void chunk_init(chunk_t* chunk);
void chunk_free(chunk_t* chunk);

void chunk_write8(chunk_t* chunk, uint8_t data, uint32_t line);
void chunk_write32(chunk_t* chunk, uint32_t data, uint32_t line);

uint32_t chunk_read32(const chunk_t* chunk, size_t offset);

void chunk_write_const(chunk_t* chunk, value_t value, uint32_t line);

size_t chunk_add_value(chunk_t* chunk, value_t value); // TODO make private?

void chunk_dump(const chunk_t* chunk);

uint32_t chunk_get_line_for_offset(const chunk_t* chunk, size_t offset);

#endif
