#ifndef _clox_vm_h_
#define _clox_vm_h_

#define VM_STACK_MAX 256

#include "common.h"
#include "value.h"

typedef struct chunk chunk_t;

typedef struct vm vm_t;

typedef enum {
    RUN_OK,
    RUN_COMPILE_ERROR,
    RUN_RUNTIME_ERROR,
} run_result_t;

vm_t* vm_create(void);
void vm_destroy(vm_t* vm);

run_result_t vm_run_source(vm_t* vm, const char* source);
run_result_t vm_run_chunk(vm_t* vm, const chunk_t* chunk);

void vm_stack_dump(const vm_t *vm);
void vm_stack_push(vm_t* vm, value_t value);
value_t vm_stack_pop(vm_t* vm);
value_t vm_stack_peek(vm_t* vm, int offset);

#endif
