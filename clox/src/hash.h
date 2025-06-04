#ifndef _clox_hash_h_
#define _clox_hash_h_

#include <stdint.h>
#include <stddef.h>

uint32_t hash_nil();
uint32_t hash_bool(bool value);
uint32_t hash_double(double value);
uint32_t hash_string(const char* start, size_t length);

uint32_t hash_bytes(const void* ptr, size_t size);

#endif