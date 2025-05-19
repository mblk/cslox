#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "vm.h"
#include "compiler.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int interpret(const char *source) {
    assert(source);

    scanner_t scanner;
    scanner_init(&scanner, source);

    uint32_t line = 0;

    for(;;) {
        const token_t token = scan_token(&scanner);

        printf("[%u] %16s '%.*s'\n", token.line, token_type_to_string(token.type), token.length, token.start);

        if (token.type == TOKEN_EOF)
            break;
    }

    #if 0
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
    #endif

    return 0;
}

static int run_repl() {
    char line[1024];

    for(;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            // ctrl+d
            printf("Bye.\n");
            break;
        }

        interpret(line);
    }

    return 0;
}

static const char *read_file(const char* filename) {
    assert(filename);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s'.\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }

    size_t bytes_read = fread(buffer, file_size, 1, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Failed to read file.\n");
        exit(EXIT_FAILURE);
    }

    buffer[bytes_read] = '\0';

    fclose(file);

    return buffer; // caller must free memory.
}

static int run_file(const char* filename) {
    assert(filename);

    const char *buffer = read_file(filename);
    assert(buffer);

    interpret(buffer);

    free((void*)buffer);

    return 0;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        return run_repl();
    } else if (argc == 2) {
        return run_file(argv[1]);
    } else {
        printf("usage:\n");
        printf("  %s [file]", argv[0]);
        return -1;
    }
    return 0;
}

int main_(const int argc, const char *const argv[const argc]) {
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
