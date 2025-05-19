#include "vm.h"
#include "chunk.h"
#include "debug.h"

#include <assert.h>
#include <stdio.h>

#define VM_TRACE_EXECUTION

void vm_init(vm_t* vm)
{
    assert(vm);

    vm->chunk = NULL;
    vm->ip = NULL;

    vm->sp = vm->stack; // sp points to next free slot
}

void vm_free(vm_t* vm)
{
    assert(vm);
}

run_result_t vm_run(vm_t* vm, const chunk_t* chunk)
{
    assert(vm);
    assert(vm->chunk == NULL);
    assert(vm->ip == NULL);

    assert(chunk);
    assert(chunk->code);
    assert(chunk->count > 0);

    vm->chunk = chunk;
    vm->ip = chunk->code;

    #define READ_BYTE()         (*(vm->ip++))
    #define READ_WORD()         (vm->ip += 2, *((uint16_t *)(vm->ip - 2)))
    #define READ_DWORD()        (vm->ip += 4, *((uint32_t *)(vm->ip - 4)))

    #define READ_CONST()        (chunk->values.values[READ_BYTE()])
    #define READ_CONST_LONG()   (chunk->values.values[READ_DWORD()])

    #define PUSH(value)         vm_stack_push(vm, value)
    #define POP()               vm_stack_pop(vm)

    #define UNARY_OP(op) \
        do { \
            value_t right = POP(); \
            value_t result = op right; \
            PUSH(result); \
        } while (false)

    #define BINARY_OP(op) \
        do { \
            value_t right = POP(); \
            value_t left = POP(); \
            value_t result = left op right; \
            PUSH(result); \
        } while (false)

    for(;;) {

        #ifdef VM_TRACE_EXECUTION
            printf("\n");
            vm_stack_dump(vm);
            printf("Next: ");
            disassemble_instruction(chunk, (size_t)(vm->ip - vm->chunk->code));
        #endif

        const uint8_t opcode = READ_BYTE();
        switch (opcode) {
        
            case OP_CONST:      PUSH(READ_CONST());      break;
            case OP_CONST_LONG: PUSH(READ_CONST_LONG()); break;

            case OP_NEGATE: UNARY_OP(-); break;

            case OP_ADD: BINARY_OP(+); break;
            case OP_SUB: BINARY_OP(-); break;
            case OP_MUL: BINARY_OP(*); break;
            case OP_DIV: BINARY_OP(/); break;

            case OP_RETURN:
            {
                value_t value = POP();
                printf("Return: %lf\n", value);
                return RUN_OK;
            }

            default:
                printf("unknown opcode %d\n", opcode);
                return RUN_RUNTIME_ERROR;
        }
    }

    #undef POP
    #undef PUSH

    #undef READ_CONST_LONG
    #undef READ_CONST

    #undef READ_DWORD
    #undef READ_WORD
    #undef READ_BYTE

    assert(!"Must not reach");
    return RUN_RUNTIME_ERROR;
}

void vm_stack_dump(const vm_t *vm)
{
    printf("Stack: [");

    for (const value_t *slot = vm->stack; slot < vm->sp; slot++) {
        printf("%lf ", *slot);
    }

    printf("] (top)\n");
}

void vm_stack_push(vm_t* vm, value_t value)
{
    assert(vm);

    //printf("push %lf\n", value);

    *vm->sp = value;
    vm->sp++;
}

value_t vm_stack_pop(vm_t* vm)
{
    assert(vm);

    vm->sp--;
    value_t value = *vm->sp;

    //printf("pop %lf\n", value);

    return value;
}