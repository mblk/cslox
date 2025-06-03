#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "compiler.h"
#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static int interpret(const char *source) {
    assert(source);

    vm_t* const vm = vm_create();
    {
        const run_result_t result = vm_run_source(vm, source);

        printf("Result: ");
        switch (result) {
            case RUN_OK:            printf("OK\n");            break;
            case RUN_COMPILE_ERROR: printf("Compile error\n"); break;
            case RUN_RUNTIME_ERROR: printf("Runtime error\n"); break;
            default:                printf("unknown error\n"); break;
        }
    }
    vm_destroy(vm);

    return 0;
}

static int run_repl(void) {
    char line[1024];

    for(;;) {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            // ctrl+d
            printf("Bye.\n");
            break;
        }

        if (strlen(line) == 1 && line[0] == '\n') {
            continue;
        }

        interpret(line);
    }

    return 0;
}

static const char *read_file(const char* filename) {
    assert(filename);

    FILE * const file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file '%s'.\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0L, SEEK_END);
    const size_t file_size = ftell(file);
    rewind(file);

    char* const buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Out of memory.\n");
        exit(EXIT_FAILURE);
    }

    const size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Failed to read file (wanted %zu, read %zu).\n", file_size, bytes_read);
        exit(EXIT_FAILURE);
    }

    buffer[bytes_read] = '\0';

    fclose(file);

    return buffer; // caller must free memory.
}

static int run_file(const char* filename) {
    assert(filename);

    const char * const buffer = read_file(filename);
    assert(buffer);

    interpret(buffer);

    free((void*)buffer);

    return 0;
}

int main_(int argc, char** argv) {
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

int main__(const int argc, const char *const argv[const argc]) {
    (void)argc;
    (void)argv;

    vm_t* const vm = vm_create();
    {
        chunk_t chunk;
        chunk_init(&chunk);

        chunk_write_const(&chunk, NUMBER_VALUE(1.2345), 1);
        chunk_write_const(&chunk, NUMBER_VALUE(5.4321), 2);

        chunk_write8(&chunk, OP_MUL, 3);

        // for (int i=0; i<500; i++) {
        //     chunk_write_const(&chunk, (double)i, 3);
        // }

        chunk_write8(&chunk, OP_RETURN, 10);

        disassemble_chunk(&chunk, "My chunk");

        printf("Running...\n");
        run_result_t result = vm_run_chunk(vm, &chunk);

        printf("Result: ");
        switch (result) {
            case RUN_OK: printf("OK\n"); break;
            case RUN_COMPILE_ERROR: printf("Compile error\n"); break;
            case RUN_RUNTIME_ERROR: printf("runtime error\n"); break;
            default: printf("unknown error\n"); break;
        }

        chunk_free(&chunk);
    }
    vm_destroy(vm);

    return 0;
}

int main(int argc, char** argv) { // table-tests
    (void)argc;
    (void)argv;

    object_root_t root;
    root.first = NULL;

    string_object_t* key1 = create_string_object(&root, "key1", 4);
    string_object_t* key2 = create_string_object(&root, "key2", 4);
    string_object_t* key3 = create_string_object(&root, "key3", 4);

    value_t value1 = NUMBER_VALUE(1.2345);
    value_t value2 = BOOL_VALUE(true);
    value_t value3 = OBJECT_VALUE((object_t*)create_string_object(&root, "value3", 6));



    table_t table;

    table_init(&table);
    //table_dump(&table, "my table before");
    table_set(&table, key1, value1);
    table_set(&table, key2, value2);
    table_set(&table, key3, value3);
    //table_dump(&table, "my table 11");
    table_delete(&table, key2);
    table_delete(&table, key1);
    table_delete(&table, key3);
    table_set(&table, key3, value3);
    table_dump(&table, "my table");

    {
        value_t value4;
        if (table_get(&table, key3, &value4)) {
            printf("get key3 -> ");
            print_value(value4);
            printf("\n");
        } else {
            printf("unable to get key3\n");
        }
    }

    table_free(&table);

    free_objects(&root);

    return 0;
}