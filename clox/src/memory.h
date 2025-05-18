#ifndef _clox_memory_h_
#define _clox_memory_h_

#include <stddef.h>

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, ptr, old_count, new_count) (type*)memory_alloc(ptr, old_count * sizeof(type), new_count * sizeof(type));

void* memory_alloc(void* ptr, size_t old_size, size_t new_size);

#endif
