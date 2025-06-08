#include "chunk.h"
#include "memory.h"
#include "value.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void chunk_init(chunk_t* chunk) {
    assert(chunk);
    
    memset(chunk, 0, sizeof(chunk_t));

    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;

    chunk->line_infos_capacity = 0;
    chunk->line_infos_count = 0;
    chunk->line_infos = NULL;

    value_array_init(&chunk->values);
}

void chunk_free(chunk_t* chunk) {
    assert(chunk);

    chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->capacity, 0);
    chunk->capacity = 0;
    chunk->count = 0;

    chunk->line_infos = GROW_ARRAY(line_info_t, chunk->line_infos, chunk->line_infos_capacity, 0);
    chunk->line_infos_capacity = 0;
    chunk->line_infos_count = 0;

    //value_array_dump(&chunk->values);
    value_array_free(&chunk->values);
}

uint8_t chunk_read8(const chunk_t* chunk, size_t offset) {
    assert(chunk);
    // TODO check bounds

    return chunk->code[offset];
}

uint32_t chunk_read32(const chunk_t* chunk, size_t offset) {
    assert(chunk);
    // TODO check bounds

    uint32_t ret = 0;
    memcpy(&ret, chunk->code + offset, sizeof(uint32_t));
    return ret;
}

void chunk_write8(chunk_t* chunk, uint8_t data, uint32_t line) {
    assert(chunk);

    if (chunk->count + 1 > chunk->capacity) {
        const size_t old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
    }

    chunk->code[chunk->count++] = data;

    // The current implementation only works if lines are ordered.
    #ifndef NDEBUG
    if (chunk->line_infos_count > 0) {
        uint32_t top_line = chunk->line_infos[chunk->line_infos_count - 1].line;
        assert(line >= top_line);
    }
    #endif

    // Add to top-entry?
    if (chunk->line_infos_count > 0 && chunk->line_infos[chunk->line_infos_count - 1].line == line) {
        chunk->line_infos[chunk->line_infos_count - 1].bytes++;
    } else {
        if (chunk->line_infos_count + 1 > chunk->line_infos_capacity) {
            const size_t old_capacity = chunk->line_infos_capacity;
            chunk->line_infos_capacity = GROW_CAPACITY(chunk->line_infos_capacity);
            chunk->line_infos = GROW_ARRAY(line_info_t, chunk->line_infos, old_capacity, chunk->line_infos_capacity);
        }

        chunk->line_infos[chunk->line_infos_count++] = (line_info_t) { .line = line, .bytes = 1 };
    }
}

void chunk_write32(chunk_t* chunk, uint32_t data, uint32_t line) {
    assert(chunk);

    const uint8_t *p = (const uint8_t*)&data;
    chunk_write8(chunk, p[0], line);
    chunk_write8(chunk, p[1], line);
    chunk_write8(chunk, p[2], line);
    chunk_write8(chunk, p[3], line);
}

uint32_t chunk_add_value(chunk_t* chunk, value_t value) {
    assert(chunk);

    // Try so reuse existing values.
    for (size_t i=0; i<chunk->values.count; i++) {
        const value_t existing_value = chunk->values.values[i];
        if (values_equal(value, existing_value)) {
            return i;
        }
    }

    // add new value.
    const uint32_t index = value_array_write(&chunk->values, value);
    return index;
}

void chunk_dump(const chunk_t *chunk) {
    assert(chunk);

    printf("== Chunk ==\n");

    printf("code count=%zu capacity=%zu\n", chunk->count, chunk->capacity);
    for (size_t i=0; i<chunk->count; i++) {
        if (i % 16 == 0) printf("%04zX: ", i);
        printf("%02X ", chunk->code[i]);
        if (i % 16 == 15) printf("\n");
    }
    printf("\n");

    printf("line info count=%zu capacity=%zu\n", chunk->line_infos_count, chunk->line_infos_capacity);
    for (size_t i=0; i<chunk->line_infos_count; i++) {
        const line_info_t li = chunk->line_infos[i];
        printf("line=%u bytes=%u\n", li.line, li.bytes);
    }

    value_array_dump(&chunk->values);
}

uint32_t chunk_get_line_for_offset(const chunk_t* chunk, size_t offset) {
    assert(chunk);

    size_t current_offset = 0;

    for (size_t i = 0; i<chunk->line_infos_count; i++) {

        if (offset < current_offset + chunk->line_infos[i].bytes) {
            return chunk->line_infos[i].line;
        }

        current_offset += chunk->line_infos[i].bytes;
    }

    // error case
    chunk_dump(chunk);
    printf("Error: can't find line for offset %zu\n", offset);

    assert(!"could not find line for offset");
    return 0;
}