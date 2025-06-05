#define _POSIX_C_SOURCE 200809L // for clock_gettime()

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
#include <time.h>

#if 1
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
#endif

#if 0
int main(const int argc, const char *const argv[const argc]) { // chunk-tests
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
#endif

#if 0
int main(int argc, char** argv) { // table-tests
    (void)argc;
    (void)argv;

    object_root_t root;
    root.first = NULL;

    const value_t key1 = OBJECT_VALUE((object_t*)create_string_object(&root, "key1", 4));
    const value_t key2 = OBJECT_VALUE((object_t*)create_string_object(&root, "key2", 4));
    const value_t key3 = OBJECT_VALUE((object_t*)create_string_object(&root, "key3", 4));
    const value_t key4 = BOOL_VALUE(false);
    const value_t key5 = BOOL_VALUE(true);
    const value_t key6 = NUMBER_VALUE(1.234);
    const value_t key7 = NUMBER_VALUE(1.235);
    const value_t key8 = NUMBER_VALUE(0.0);

    value_t value1 = NUMBER_VALUE(1.2345);
    value_t value2 = BOOL_VALUE(true);
    value_t value3 = OBJECT_VALUE((object_t*)create_string_object(&root, "value3", 6));
    value_t value4 = OBJECT_VALUE((object_t*)create_string_object(&root, "value4", 6));

    table_t table;
    table_init(&table);
    {
        //table_dump(&table, "my table before");

        table_set(&table, key1, value1);
        table_set(&table, key2, value2);
        table_set(&table, key3, value3);

        table_set(&table, key4, value2);
        table_set(&table, key5, value3);

        table_set(&table, key6, value2);
        table_set(&table, key7, value3);

        table_set(&table, key8, value4);


        //table_dump(&table, "my table 11");
        //table_delete(&table, key2);
        //table_delete(&table, key1);
        //table_delete(&table, key3);
        //table_set(&table, key3, value3);
        table_dump(&table, "my table");
    
        {
            value_t value4;
    
            if (table_get_by_string(&table, "key1", 4, NULL, &value4)) {
                printf("table_get: ");
                print_value(value4);
                printf("\n");
            } else {
                printf("unable to get value\n");
            }
        }
    }
    table_free(&table);

    object_root_free(&root);

    return 0;
}
#endif

#if 0

static inline uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static double run_table_benchmark(size_t N);

int main(int argc, char** argv) { // table-benchmark
    (void)argc;
    (void)argv;

    const size_t repeats = 3;

    const size_t sizes[] = {
        1000,
        1000 * 10,
        1000 * 100,
        1000 * 1000,
        1000 * 1000 * 10,
        //1000 * 1000 * 100,
        //1000 * 1000 * 1000,
    };
    const size_t num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t i=0; i<num_sizes; i++) {
        const size_t N = sizes[i];

        for (size_t r=0; r<repeats; r++) {
            const double t = run_table_benchmark(N);
        
            printf("N=%zu t=%.6lf\n", N, t);
        }
    }

    return 0;
}

static double run_table_benchmark(size_t N) {

    object_root_t root;
    object_root_init(&root);

    table_t table;
    table_init(&table);

    double* keys = malloc(sizeof(double) * N);
    assert(keys);

    for (size_t i=0; i<N; i++) {
        //const double key = (double)rand(); // Note: there might be duplicates

        const double key = (double)i * 2;

        //uint64_t key_bits = (uint64_t)i * 2654435761ULL; // Streuung mit Knuth's Multiplikator
        //const double key = (double)key_bits;

        const value_t key_value = NUMBER_VALUE(key);
        
        keys[i] = key;
        table_set(&table, key_value, key_value);
    }

    // --- start of timing: only lookup + value-check ---
    uint64_t t_start = get_time_ns();

    volatile double sum = 0;

    for (size_t i=0; i<N; i++) {
        const double x = keys[i];
        value_t value = NIL_VALUE();

        const bool found = table_get(&table, NUMBER_VALUE(x), &value);
        if (!found) {
            printf("not found\n");
            return -1;
        }

        if (!values_equal(NUMBER_VALUE(x), value)) {
            printf("key != value\n");
            return -1;
        }

        sum += AS_NUMBER(value);
    }

    // --- end of timing ---
    uint64_t t_end = get_time_ns();
    double elapsed_sec = (t_end - t_start) / 1e9;

    //printf("N=%zu, dt=%.6lfs      %.1lf\n", N, elapsed_sec, sum);

    if (sum == 123) { // prevent optimization
        printf("sum was 123\n");
    }

    free(keys);
    table_free(&table);
    object_root_free(&root);

    return elapsed_sec;
}
#endif