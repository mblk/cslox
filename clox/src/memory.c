#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

void* memory_alloc(void* ptr, size_t old_size, size_t new_size)
{
    (void)old_size;

    #if 0
    printf("memory_alloc ptr=%p old_size=%zu new_size=%zu\n", ptr, old_size, new_size);
    #endif

    if (new_size == 0) {
        if (ptr) {
            free(ptr);
        }
        return NULL;
    }

    return realloc(ptr, new_size);
}