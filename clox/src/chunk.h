#ifndef _clox_chunk_h_
#define _clox_chunk_h_

#include "value.h"

#include <stdint.h>

typedef enum {              // args:
    OP_INVALID,

    OP_CONST,               //  8 bit index
    OP_CONST_LONG,          // 32 bit index

    OP_NIL,                 // -
    OP_TRUE,                // -
    OP_FALSE,               // -

    OP_NOT,                 // -
    OP_NEGATE,              // -

    OP_EQUAL,               // -
    OP_GREATER,             // -
    OP_LESS,                // -
    // direct:
    // a == b
    // a > b
    // a < b
    // indirect:
    // a != b   via   !(a == b)
    // a >= b   via   !(a < b)
    // a <= b   via   !(a > b)

    OP_ADD,         // -
    OP_SUB,         // -
    OP_MUL,         // -
    OP_DIV,         // -

    OP_DEFINE_GLOBAL,       //  8 bit index to value-table for name
    OP_DEFINE_GLOBAL_LONG,  // 32 bit index to value-table for name
    OP_GET_GLOBAL,          //  8 bit index to value-table for name
    OP_GET_GLOBAL_LONG,     // 32 bit index to value-table for name
    OP_SET_GLOBAL,          //  8 bit index to value-table for name
    OP_SET_GLOBAL_LONG,     // 32 bit index to value-table for name

    OP_GET_LOCAL,           // 8  bit index to stack
    OP_GET_LOCAL_LONG,      // 32 bit index to stack
    OP_SET_LOCAL,           // 8  bit index to stack
    OP_SET_LOCAL_LONG,      // 32 bit index to stack

    OP_GET_UPVALUE,         //  8 bit index
    OP_GET_UPVALUE_LONG,    // 32 bit index
    OP_SET_UPVALUE,         //  8 bit index
    OP_SET_UPVALUE_LONG,    // 32 bit index

    OP_JUMP,                // 16 bit signed offset
    OP_JUMP_IF_TRUE,        // 16 bit signed offset
    OP_JUMP_IF_FALSE,       // 16 bit signed offset
    
    OP_POP,                 // -

    OP_CALL,                // 8 bit argument count
    OP_RETURN,              // -

    OP_CLOSURE,             // 8 bit index to value-table for function-object, followed by upvalue-pairs (1 byte type, 1 byte index)
    OP_CLOSE_UPVALUE,       // -
    
    OP_PRINT,               // -

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

uint8_t chunk_read8(const chunk_t* chunk, size_t offset);
uint32_t chunk_read32(const chunk_t* chunk, size_t offset);

void chunk_write8(chunk_t* chunk, uint8_t data, uint32_t line);
void chunk_write32(chunk_t* chunk, uint32_t data, uint32_t line);

uint32_t chunk_add_value(chunk_t* chunk, value_t value);

void chunk_dump(const chunk_t* chunk);

uint32_t chunk_get_line_for_offset(const chunk_t* chunk, size_t offset);

#endif
