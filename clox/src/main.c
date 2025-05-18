#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"

#include <stdio.h>

int main(const int argc, const char *const argv[const argc])
{
    (void)argc;
    (void)argv;

    vm_t vm;
    vm_init(&vm);
    {
        chunk_t chunk;
        chunk_init(&chunk);

        chunk_write_const(&chunk, 1.2345, 1);
        chunk_write_const(&chunk, 5.4321, 2);

        chunk_write8(&chunk, OP_MUL, 3);

        // for (int i=0; i<500; i++) {
        //     chunk_write_const(&chunk, (double)i, 3);
        // }

        chunk_write8(&chunk, OP_RETURN, 10);

        disassemble_chunk(&chunk, "My chunk");

        printf("Running...\n");
        run_result_t result = vm_run(&vm, &chunk);

        printf("Result: ");
        switch (result) {
            case RUN_OK: printf("OK\n"); break;
            case RUN_COMPILE_ERROR: printf("Compile error\n"); break;
            case RUN_RUNTIME_ERROR: printf("runtime error\n"); break;
            default: printf("unknown error\n"); break;
        }

        chunk_free(&chunk);
    }
    vm_free(&vm);

    return 0;
}
