#ifndef _clox_memory_h_
#define _clox_memory_h_

#include <stddef.h>

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)

#define ALLOC_BY_COUNT(type, count)                 (type*)memory_alloc(NULL, 0, sizeof(type) * count)
#define ALLOC_BY_SIZE(type, size)                   (type*)memory_alloc(NULL, 0, size)
#define GROW_ARRAY(type, ptr, old_count, new_count) (type*)memory_alloc((void*)ptr, old_count * sizeof(type), new_count * sizeof(type))

#define FREE_BY_COUNT(type, ptr, count)             memory_alloc((void*)ptr, sizeof(type) * count, 0)
#define FREE_BY_SIZE(ptr, size)                     memory_alloc((void*)ptr, size, 0)
#define FREE(type, ptr)                             memory_alloc((void*)ptr, sizeof(type), 0)

void* memory_alloc(void* ptr, size_t old_size, size_t new_size);

#endif
