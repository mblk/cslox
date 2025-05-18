#include "debug.h"
#include "chunk.h"
#include "value.h"

#include <stdio.h>
#include <assert.h>

void disassemble_chunk(const chunk_t* chunk, const char *name)
{
    assert(chunk);
    assert(name);

    printf("== %s ==\n", name);
    printf("addr line opcode              arguments\n");

    for (size_t offset=0; offset < chunk->count; ) {
        offset += disassemble_instruction(chunk, offset);
    }
}

static uint32_t find_line_for_offset(const chunk_t* chunk, size_t offset)
{
    assert(chunk);

    size_t current_offset = 0;

    for (size_t i = 0; i<chunk->line_infos_count; i++) {

        if (offset < current_offset + chunk->line_infos[i].bytes) {
            return chunk->line_infos[i].line;
        }

        current_offset += chunk->line_infos[i].bytes;
    }

    assert(!"could not find line for offset");
    return 0;
}

static void print_value(value_t value)
{
    printf("%lf", value);
}

static size_t constant_instruction(const chunk_t* chunk, const char* name, size_t offset)
{
    const uint8_t index = chunk->code[offset + 1];
    const value_t value = chunk->values.values[index];

    printf("%-16s %4d '", name, index);
    print_value(value);
    printf("'\n");

    return 2;
}

static size_t long_constant_instruction(const chunk_t* chunk, const char* name, size_t offset)
{
    const uint32_t index = chunk_read32(chunk, offset + 1);
    const value_t value = chunk->values.values[index];

    printf("%-16s %4d '", name, index);
    print_value(value);
    printf("'\n");

    return 1 + 4;
}

static size_t simple_instruction(const char *name)
{
    printf("%s\n", name);
    return 1;
}

static size_t unknown_instruction(uint8_t opcode)
{
    printf("Unknown opcode %02X\n", opcode);
    return 1;
}

size_t disassemble_instruction(const chunk_t* chunk, size_t offset)
{
    assert(chunk);

    printf("%04zX ", offset);

    const uint32_t line = find_line_for_offset(chunk, offset);
    const uint32_t prev_line = offset > 0 ? find_line_for_offset(chunk, offset - 1) : 0;

    if (line == prev_line) {
        printf("   | ");
    } else {
        printf("%4u ", line);
    }

    const uint8_t opcode = chunk->code[offset];
    switch (opcode) {
        case OP_CONST:      return constant_instruction(chunk, "OP_CONST", offset);
        case OP_CONST_LONG: return long_constant_instruction(chunk, "OP_CONST_LONG", offset);
        case OP_NEGATE:     return simple_instruction("OP_NEGATE");
        case OP_ADD:        return simple_instruction("OP_ADD");
        case OP_SUB:        return simple_instruction("OP_SUB");
        case OP_MUL:        return simple_instruction("OP_MUL");
        case OP_DIV:        return simple_instruction("OP_DIV");
        case OP_RETURN:     return simple_instruction("OP_RETURN");
        default:            return unknown_instruction(opcode);
    }
}