#include "vm.h"
#include "chunk.h"
#include "debug.h"
#include "compiler.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#define VM_TRACE_EXECUTION
#define VM_PRINT_CODE

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

static void reset_stack(vm_t* vm)
{
    // TODO
}

static void runtime_error(vm_t* vm, const char* format, ...)
{
    const size_t offset = vm->ip - vm->chunk->code - 1;
    const uint32_t line = chunk_get_line_for_offset(vm->chunk, offset);
    fprintf(stderr, "[Line %u] Runtime error: ", line);
 
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    reset_stack(vm);
}

run_result_t vm_run_source(vm_t* vm, const char* source)
{
    run_result_t result = RUN_OK;

    chunk_t chunk;
    chunk_init(&chunk);
    {
        if (compile(&chunk, source)) {

            #ifdef VM_PRINT_CODE
                disassemble_chunk(&chunk, "Code");
            #endif

            result = vm_run_chunk(vm, &chunk);
        } else {
            result = RUN_COMPILE_ERROR;
        }
    }
    chunk_free(&chunk);

    return result;
}

run_result_t vm_run_chunk(vm_t* vm, const chunk_t* chunk)
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
    #define PEEK(offset)        vm_stack_peek(vm, offset)

    #define ERROR(args...)      runtime_error(vm, args)

    #define UNARY_NUMBER_OP(op) \
        do { \
            if (!IS_NUMBER(PEEK(0))) { \
                ERROR("Operand must be number"); \
                return RUN_RUNTIME_ERROR; \
            } \
            value_t right = POP(); \
            value_t result = NUMBER_VALUE(op AS_NUMBER(right)); \
            PUSH(result); \
        } while (false)

    #define BINARY_NUMBER_OP(op) \
        do { \
            if (!IS_NUMBER(PEEK(1)) || !IS_NUMBER(PEEK(1))) { \
                ERROR("Operands must be numbers"); \
                return RUN_RUNTIME_ERROR; \
            } \
            value_t right = POP(); \
            value_t left = POP(); \
            value_t result = NUMBER_VALUE(AS_NUMBER(left) op AS_NUMBER(right)); \
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

            case OP_NIL:   PUSH(NIL_VALUE());       break;
            case OP_TRUE:  PUSH(BOOL_VALUE(true));  break;
            case OP_FALSE: PUSH(BOOL_VALUE(false)); break;

            case OP_NOT: PUSH(BOOL_VALUE(value_is_falsey(POP()))); break;
            case OP_NEGATE: UNARY_NUMBER_OP(-); break;

            case OP_ADD: BINARY_NUMBER_OP(+); break;
            case OP_SUB: BINARY_NUMBER_OP(-); break;
            case OP_MUL: BINARY_NUMBER_OP(*); break;
            case OP_DIV: BINARY_NUMBER_OP(/); break;

            case OP_RETURN:
            {
                value_t value = POP();
                printf("Return: ");
                print_value(value);
                printf("\n");
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
        print_value(*slot);
        printf(" ");
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

value_t vm_stack_peek(vm_t* vm, int offset)
{
    assert(offset >= 0);
    // sp[0] -> next free slot
    // sp[-1] -> most recent value
    // sp[-2] -> second most recent value
    // ...
    return vm->sp[-1 - offset];
}