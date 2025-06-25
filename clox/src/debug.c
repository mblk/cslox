#include "debug.h"
#include "chunk.h"
#include "compiler.h"
#include "object.h"
#include "value.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

void disassemble_chunk(const chunk_t* chunk, const char *name) {
    assert(chunk);
    assert(name);

    printf("== %s ==\n", name);
    printf("addr line opcode              arguments\n");

    for (size_t offset=0; offset < chunk->count; ) {
        offset += disassemble_instruction(chunk, offset);
    }
}

static size_t constant_instruction(const chunk_t* chunk, const char* name, size_t offset) {
    const uint8_t index = chunk_read8(chunk, offset + 1);
    const value_t value = chunk->values.values[index];

    printf("%-20s %4d '", name, index);
    print_value(value);
    printf("'\n");

    return 1 + 1;
}

static size_t long_constant_instruction(const chunk_t* chunk, const char* name, size_t offset) {
    const uint32_t index = chunk_read32(chunk, offset + 1);
    const value_t value = chunk->values.values[index];

    printf("%-20s %4d '", name, index);
    print_value(value);
    printf("'\n");

    return 1 + 4;
}

static size_t byte_instruction(const chunk_t* chunk, const char* name, size_t offset) {
    const uint8_t stack_index = chunk_read8(chunk, offset + 1);

    printf("%-20s %4d\n", name, stack_index);

    return 1 + 1;
}

static size_t byte_instruction_long(const chunk_t* chunk, const char* name, size_t offset) {
    const uint32_t stack_index = chunk_read32(chunk, offset + 1);
    
    printf("%-20s %4d\n", name, stack_index);

    return 1 + 4;
}

static size_t jump_instruction(const chunk_t* chunk, const char* name, size_t offset) {
    int16_t jump_offset = 0;
    memcpy(&jump_offset, chunk->code + offset + 1, sizeof(int16_t));

    printf("%-20s %+4d (+3 = %+d)\n", name, jump_offset, jump_offset + 3);

    return 1 + 2;
}

static size_t simple_instruction(const char *name) {
    printf("%s\n", name);
    return 1;
}

static size_t closure_instruction(const chunk_t* chunk, const char* name, size_t offset) {
    offset++;
    const uint8_t const_index = chunk_read8(chunk, offset++);
    const value_t const_value = chunk->values.values[const_index];

    assert(IS_FUNCTION(const_value));
    const function_object_t* const function = AS_FUNCTION(const_value);

    printf("%-20s %4d '", name, const_index);
    print_value(const_value);
    printf("'\n");

    for (size_t i=0; i<function->upvalue_count; i++) {
        printf("%04zd    |                         ", offset);
        const uint8_t is_local = chunk_read8(chunk, offset++);
        const uint8_t index = chunk_read8(chunk, offset++);
        printf("%s %u\n", is_local == 1 ? "local" : "upvalue", index);
    }

    return 1 + 1 + function->upvalue_count * 2;
}

static size_t unknown_instruction(uint8_t opcode) {
    printf("Unknown opcode %02X\n", opcode);
    return 1;
}

size_t disassemble_instruction(const chunk_t* chunk, size_t offset) {
    assert(chunk);

    printf("%04zd ", offset);

    const uint32_t line = chunk_get_line_for_offset(chunk, offset);
    const uint32_t prev_line = offset > 0 ? chunk_get_line_for_offset(chunk, offset - 1) : 0;

    if (line == prev_line) {
        printf("   | ");
    } else {
        printf("%4u ", line);
    }

    const uint8_t opcode = chunk->code[offset];
    switch (opcode) {
        case OP_CONST:      return constant_instruction(chunk, "OP_CONST", offset);
        case OP_CONST_LONG: return long_constant_instruction(chunk, "OP_CONST_LONG", offset);

        case OP_NIL:        return simple_instruction("OP_NIL");
        case OP_TRUE:       return simple_instruction("OP_TRUE");
        case OP_FALSE:      return simple_instruction("OP_FALSE");

        case OP_NOT:        return simple_instruction("OP_NOT");
        case OP_NEGATE:     return simple_instruction("OP_NEGATE");

        case OP_EQUAL:      return simple_instruction("OP_EQUAL");
        case OP_GREATER:    return simple_instruction("OP_GREATER");
        case OP_LESS:       return simple_instruction("OP_LESS");

        case OP_ADD:        return simple_instruction("OP_ADD");
        case OP_SUB:        return simple_instruction("OP_SUB");
        case OP_MUL:        return simple_instruction("OP_MUL");
        case OP_DIV:        return simple_instruction("OP_DIV");

        case OP_DEFINE_GLOBAL:      return constant_instruction(chunk, "OP_DEFINE_GLOBAL", offset);
        case OP_DEFINE_GLOBAL_LONG: return long_constant_instruction(chunk, "OP_DEFINE_GLOBAL_LONG", offset);
        case OP_GET_GLOBAL:         return constant_instruction(chunk, "OP_GET_GLOBAL", offset);
        case OP_GET_GLOBAL_LONG:    return long_constant_instruction(chunk, "OP_GET_GLOBAL_LONG", offset);
        case OP_SET_GLOBAL:         return constant_instruction(chunk, "OP_SET_GLOBAL", offset);
        case OP_SET_GLOBAL_LONG:    return long_constant_instruction(chunk, "OP_SET_GLOBAL_LONG", offset);

        case OP_GET_LOCAL:      return byte_instruction(chunk, "OP_GET_LOCAL", offset);
        case OP_GET_LOCAL_LONG: return byte_instruction_long(chunk, "OP_GET_LOCAL_LONG", offset);
        case OP_SET_LOCAL:      return byte_instruction(chunk, "OP_SET_LOCAL", offset);
        case OP_SET_LOCAL_LONG: return byte_instruction_long(chunk, "OP_SET_LOCAL_LONG", offset);

        case OP_GET_UPVALUE:        return byte_instruction(chunk, "OP_GET_UPVALUE", offset);
        case OP_GET_UPVALUE_LONG:   return byte_instruction_long(chunk, "OP_GET_UPVALUE_LONG", offset);
        case OP_SET_UPVALUE:        return byte_instruction(chunk, "OP_SET_UPVALUE", offset);
        case OP_SET_UPVALUE_LONG:   return byte_instruction_long(chunk, "OP_SET_UPVALUE_LONG", offset);

        case OP_JUMP:           return jump_instruction(chunk, "OP_JUMP", offset);
        case OP_JUMP_IF_TRUE:   return jump_instruction(chunk, "OP_JUMP_IF_TRUE", offset);
        case OP_JUMP_IF_FALSE:  return jump_instruction(chunk, "OP_JUMP_IF_FALSE", offset);

        case OP_POP:            return simple_instruction("OP_POP");

        case OP_CALL:           return byte_instruction(chunk, "OP_CALL", offset);
        case OP_RETURN:         return simple_instruction("OP_RETURN");

        case OP_CLOSURE:        return closure_instruction(chunk, "OP_CLOSURE", offset);
        case OP_CLOSE_UPVALUE:  return simple_instruction("OP_CLOSE_UPVALUE");

        case OP_PRINT:          return simple_instruction("OP_PRINT");

        default:                return unknown_instruction(opcode);
    }
}