#define _POSIX_C_SOURCE 200809L // for clock_gettime()

#include "vm.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

static int interpret(vm_t* vm, const char *source) {
    assert(vm);
    assert(source);

    const run_result_t result = vm_run_source(vm, source);

    (void)result;

    // printf("Result: ");
    // switch (result) {
    //     case RUN_OK:            printf("OK\n");            break;
    //     case RUN_COMPILE_ERROR: printf("Compile error\n"); break;
    //     case RUN_RUNTIME_ERROR: printf("Runtime error\n"); break;
    //     default:                printf("unknown error\n"); break;
    // }

    return 0;
}

static int run_repl(void) {
    char line[1024];

    vm_t* const vm = vm_create();

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

        interpret(vm, line);
    }

    vm_destroy(vm);

    return 0;
}

static const char *read_file(const char* filename) {
    assert(filename);

    FILE* const file = fopen(filename, "rb");
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

    vm_t* const vm = vm_create();
    interpret(vm, buffer);
    vm_destroy(vm);

    free((void*)buffer);

    return 0;
}

static int scan_file(const char* filename) {
    assert(filename);

    const char * const buffer = read_file(filename);
    assert(buffer);

    scanner_t scanner;
    scanner_init(&scanner, buffer);

    for(;;) {
        const token_t token = scan_token(&scanner);
        printf("%s '%.*s'\n", token_type_to_string(token.type), token.length, token.start);
        if (token.type == TOKEN_EOF) break;
    }
        
    free((void*)buffer);

    return 0;
}

static int parse_file(const char* filename) {
    printf("parse file: %s\n", filename);
    // TODO parse and print something
    return 0;
}

static int print_usage(const char* name) {
    printf("usage:\n");
    printf("  %s [file]             Run file\n", name);
    printf("  %s                    Start REPL\n", name);
    printf("  %s -scan [file]       Scan file and print tokens\n", name);
    printf("  %s -parse [file]      Parse file\n", name);
    return 0;
}

int main(int argc, char** argv) {
    if (argc == 1) {
        return run_repl();
    } else if (argc == 2 && argv[1][0] != '-') {
        return run_file(argv[1]);
    } else if (argc == 3 && strcmp(argv[1], "-scan") == 0) {
        return scan_file(argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "-parse") == 0) {
        return parse_file(argv[2]);
    } else {
        return print_usage(argv[0]);
    }
    return 0;
}
