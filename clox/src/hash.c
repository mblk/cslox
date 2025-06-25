#include "hash.h"

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

uint32_t hash_nil() {
    return 42;
}

uint32_t hash_bool(bool value) {
    return value ? 1 : 0;
}

uint32_t hash_double(double value) {
    return hash_bytes(&value, sizeof(double));
}

uint32_t hash_string(const char* start, size_t length) {
    return hash_bytes(start, length);
}

uint32_t hash_bytes(const void* ptr, size_t size) {
    assert(ptr);
    // Size is allowed to be 0

    // FNV-1a
    // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a

    uint32_t hash = 2166136261u;

    const uint8_t* data = ptr;

    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 16777619;
    }

    return hash;
}