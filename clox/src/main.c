#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"

#include <stdio.h>

int main(int argc, char **argv)
//int main(int argc, const char * const * const argv)
//int main(const int argc, const char *const argv[const argc])
{
    (void)argc;
    (void)argv;

    chunk_t chunk;

    chunk_init(&chunk);

    size_t v1 = chunk_add_value(&chunk, 1.2);
    size_t v2 = chunk_add_value(&chunk, 5.4321);

    chunk_write8(&chunk, OP_CONST, 10);
    chunk_write8(&chunk, (uint8_t)v1, 10);

    chunk_write8(&chunk, OP_CONST, 11);
    chunk_write8(&chunk, (uint8_t)v2, 11);

    chunk_write8(&chunk, OP_CONST, 11);
    chunk_write8(&chunk, (uint8_t)v2, 11);

    for (int i=0; i<500; i++) {
        chunk_write_const(&chunk, (double)i, 13);
    }

    chunk_write8(&chunk, OP_RETURN, 14);

    
    chunk_dump(&chunk);

    disassemble_chunk(&chunk, "My chunk");

    chunk_free(&chunk);

    return 0;
}
