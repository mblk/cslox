#include "vm.h"
#include "chunk.h"
#include "debug.h"
#include "compiler.h"
#include "table.h"
#include "value.h"
#include "object.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define VM_TRACE_EXECUTION

#define VM_PRINT_CODE

typedef struct vm {
    const chunk_t* chunk;
    const uint8_t* ip;

    value_t stack[VM_STACK_MAX];
    value_t* sp;

    object_root_t root;
    table_t globals;
} vm_t;

vm_t* vm_create(void) {
    vm_t *vm = (vm_t*)malloc(sizeof(vm_t));
    assert(vm);

    memset(vm, 0, sizeof(vm_t));

    vm->chunk = NULL;
    vm->ip = NULL;

    vm->sp = vm->stack; // sp points to next free slot

    object_root_init(&vm->root);
    table_init(&vm->globals);

    return vm;
}

void vm_destroy(vm_t* vm) {
    assert(vm);

    //table_dump(&vm->globals, "VM globals");
    table_free(&vm->globals);

    //object_root_dump(&vm->root, "VM objects");
    object_root_free(&vm->root);

    free(vm);
}

static void reset_stack(vm_t* vm) {
    (void)vm;
    // TODO
}

static void runtime_error(vm_t* vm, const char* format, ...) {
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

run_result_t vm_run_source(vm_t* vm, const char* source) {
    run_result_t result = RUN_OK;

    chunk_t chunk;
    chunk_init(&chunk);
    {
        if (compile(&vm->root, &chunk, source)) {

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

static void concatenate(vm_t* vm) {
    const value_t right_value = vm_stack_pop(vm);
    const value_t left_value = vm_stack_pop(vm);
    assert(IS_STRING(left_value));
    assert(IS_STRING(right_value));

    const string_object_t* left = AS_STRING(left_value);
    const string_object_t* right = AS_STRING(right_value);
    const size_t length = left->length + right->length;
    
    // Shortcut for emtpy + empty
    if (length == 0) {
        const string_object_t* result = create_string_object(&vm->root, "", 0);
        assert(result);
        vm_stack_push(vm, OBJECT_VALUE((object_t*)result));
        return;
    }

    // concatenate strings in temporary buffer (either on stack or heap)
    constexpr size_t stack_buffer_size = 256;
    const bool use_heap = length + 1 > stack_buffer_size;
    // ie.: 
    // ...
    // 255 -> stack
    // 256 -> heap (no space for terminator)
    // ...

    char stack_buffer[stack_buffer_size];
    char* heap_buffer = nullptr;

    if (use_heap) {
        heap_buffer = malloc(length + 1);
        assert(heap_buffer);
    }

    char* const buffer = use_heap ? heap_buffer : stack_buffer;
    memcpy(buffer, left->chars, left->length);
    memcpy(buffer + left->length, right->chars, right->length);
    buffer[length] = 0; // terminate for safety and debugging, not strictly needed.

    const string_object_t* result = create_string_object(&vm->root, buffer, length);
    assert(result);

    if (use_heap) {
        assert(heap_buffer);
        free(heap_buffer);
    }

    vm_stack_push(vm, OBJECT_VALUE((object_t*)result));
}

#ifndef NDEBUG
static void vm_check_bounds(const vm_t* vm, size_t bytes_to_read) {
    const uint8_t* const start = vm->chunk->code;
    const uint8_t* const end = vm->chunk->code + vm->chunk->count;

    if (vm->ip < start) {
        printf("Error: IP before chunk start: start=%p end=%p ip=%p bytes_to_read=%zu",
            (void*)start, (void*)end, (void*)vm->ip, bytes_to_read);
        assert(!"IP before chunk start");
    }

    if (vm->ip + bytes_to_read > end) {
        printf("Error: IP after chunk end. start=%p end=%p ip=%p bytes_to_read=%zu",
            (void*)start, (void*)end, (void*)vm->ip, bytes_to_read);
        assert(!"IP after chunk end");
    }
}
#endif

run_result_t vm_run_chunk(vm_t* vm, const chunk_t* chunk) {
    assert(vm);
    assert(vm->chunk == NULL);
    assert(vm->ip == NULL);

    assert(chunk);
    assert(chunk->code);
    assert(chunk->count > 0);

    vm->chunk = chunk;
    vm->ip = chunk->code;

    // cleanup before returning
    #define RETURN(value) do { \
        vm->chunk = NULL; \
        vm->ip = NULL; \
        return value; \
    } while (false)

    #define ERROR(args...) do { \
        runtime_error(vm, args); \
        RETURN(RUN_RUNTIME_ERROR); \
    } while (false)

    #ifndef NDEBUG
    #define CHECK_IP_BOUNDS(read_size) vm_check_bounds(vm, read_size)
    #else
    #define CHECK_IP_BOUNDS(read_size) ((void)0)
    #endif

    // memcpy to prevent unaligned reads (works on x86 but is UB according to C).
    // using feature "statement expression", works on gcc/clang but not on some others like msvc.
    #define READ_TYPE(type) ({ \
        const size_t _size = sizeof(type); \
        CHECK_IP_BOUNDS(_size); \
        type _val; \
        memcpy(&_val, vm->ip, _size); \
        vm->ip += _size; \
        _val; \
    })
    #define READ_INT16()    READ_TYPE(int16_t)
    #define READ_UINT32()   READ_TYPE(uint32_t)
    
    #ifndef NDEBUG
    #define READ_BYTE()     READ_TYPE(uint8_t)
    #else
    #define READ_BYTE()     (*(vm->ip++))
    #endif

    // returns value_t
    #define READ_CONST()        (chunk->values.values[READ_BYTE()])
    #define READ_CONST_LONG()   (chunk->values.values[READ_UINT32()])

    #define PUSH(value)         vm_stack_push(vm, value)
    #define POP()               vm_stack_pop(vm)
    #define PEEK(offset)        vm_stack_peek(vm, offset)

    #define UNARY_NUMBER_OP(op) \
        do { \
            if (!IS_NUMBER(PEEK(0))) { \
                ERROR("Operand must be number"); \
            } \
            value_t right = POP(); \
            value_t result = NUMBER_VALUE(op AS_NUMBER(right)); \
            PUSH(result); \
        } while (false)

    #define BINARY_FN_OP(result_type, fn) \
        do { \
            value_t right = POP(); \
            value_t left = POP(); \
            value_t result = result_type(fn(left, right)); \
            PUSH(result); \
        } while (false)

    #define BINARY_NUMBER_OP(result_type, op) \
        do { \
            if (!IS_NUMBER(PEEK(1)) || !IS_NUMBER(PEEK(1))) { \
                ERROR("Operands must be numbers"); \
            } \
            value_t right = POP(); \
            value_t left = POP(); \
            value_t result = result_type(AS_NUMBER(left) op AS_NUMBER(right)); \
            PUSH(result); \
        } while (false)

    for(;;) {
        #ifdef VM_TRACE_EXECUTION
        {
            printf("\n");
            vm_stack_dump(vm);
            printf("Next: ");
            disassemble_instruction(chunk, (size_t)(vm->ip - vm->chunk->code));
        }
        #endif

        const uint8_t opcode = READ_BYTE();
        switch (opcode) {
        
            case OP_CONST:      PUSH(READ_CONST());      break;
            case OP_CONST_LONG: PUSH(READ_CONST_LONG()); break;

            case OP_NIL:        PUSH(NIL_VALUE());       break;
            case OP_TRUE:       PUSH(BOOL_VALUE(true));  break;
            case OP_FALSE:      PUSH(BOOL_VALUE(false)); break;

            case OP_NOT:        PUSH(BOOL_VALUE(value_is_falsey(POP()))); break;
            case OP_NEGATE:     UNARY_NUMBER_OP(-); break;

            case OP_EQUAL:      BINARY_FN_OP(BOOL_VALUE, values_equal); break;
            case OP_GREATER:    BINARY_NUMBER_OP(BOOL_VALUE, >); break;
            case OP_LESS:       BINARY_NUMBER_OP(BOOL_VALUE, <); break;

            case OP_ADD: {
                const value_t left = PEEK(1);
                const value_t right = PEEK(0);

                if (IS_STRING(left) && IS_STRING(right)) {
                    concatenate(vm);
                } else if (IS_NUMBER(left) && IS_NUMBER(right)) {
                    BINARY_NUMBER_OP(NUMBER_VALUE, +);
                } else {
                    ERROR("Operands must be two numbers or two strings.");
                }
                break;
            }
            case OP_SUB: BINARY_NUMBER_OP(NUMBER_VALUE, -); break;
            case OP_MUL: BINARY_NUMBER_OP(NUMBER_VALUE, *); break;
            case OP_DIV: BINARY_NUMBER_OP(NUMBER_VALUE, /); break;

            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                // next byte is index to value-table which contains the name
                // value for initialization is on the stack
                value_t name = opcode == OP_DEFINE_GLOBAL ? READ_CONST() : READ_CONST_LONG();
                value_t value = POP();
                table_set(&vm->globals, name, value);
                break;
            }

            case OP_GET_GLOBAL:
            case OP_GET_GLOBAL_LONG: {
                // next byte is index to value-table which contains the name
                // value is pusht onto stack
                value_t name = opcode == OP_GET_GLOBAL ? READ_CONST() : READ_CONST_LONG();
                value_t value = NIL_VALUE();

                if (!table_get(&vm->globals, name, &value)) {
                    char buffer[128] = {0};
                    print_value_to_buffer(buffer, sizeof(buffer), name);
                    ERROR("Undefined variable '%s'.", buffer);
                }

                PUSH(value);
                break;
            }

            case OP_SET_GLOBAL:
            case OP_SET_GLOBAL_LONG: {
                // next byte is index to value-table which contains the name
                // value for assignment is on the stack
                // because assignment is an expression, the value is left on the stack
                value_t name = opcode == OP_SET_GLOBAL ? READ_CONST() : READ_CONST_LONG();
                value_t value = PEEK(0);
                
                // report error if global did not exist yet
                if (table_set(&vm->globals, name, value)) {
                    table_delete(&vm->globals, name);
                    char buffer[128] = {0};
                    print_value_to_buffer(buffer, sizeof(buffer), name);
                    ERROR("Undefined variable '%s'", buffer);
                }

                break;
            }

            case OP_GET_LOCAL:
            case OP_GET_LOCAL_LONG: {
                const uint32_t stack_index = opcode == OP_GET_LOCAL ? READ_BYTE() : READ_UINT32();
                
                if (stack_index >= VM_STACK_MAX) {
                    ERROR("Stack overflow while getting local at index %u", stack_index);
                }

                const value_t value = vm->stack[stack_index];
                PUSH(value);
                break;
            }

            case OP_SET_LOCAL:
            case OP_SET_LOCAL_LONG: {
                const uint32_t stack_index = opcode == OP_SET_LOCAL ? READ_BYTE() : READ_UINT32();

                if (stack_index >= VM_STACK_MAX) {
                    ERROR("Stack overflow while setting local at index %u", stack_index);
                }

                const value_t value = PEEK(0); // leave on stack
                vm->stack[stack_index] = value;
                break;
            }

            case OP_JUMP: {
                const int16_t offset = READ_INT16();
                vm->ip = vm->ip + offset;
                break;
            }
            case OP_JUMP_IF_TRUE: {
                const int16_t offset = READ_INT16();
                if (value_is_truey(PEEK(0))) { // leave on stack
                    vm->ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE: {
                const int16_t offset = READ_INT16();
                if (value_is_falsey(PEEK(0))) { // leave on stack
                    vm->ip += offset;
                }
                break;
            }

            case OP_DUP: PUSH(PEEK(0)); break;
            case OP_POP: POP(); break;

            case OP_RETURN: {
                assert(vm->sp == vm->stack); // make sure nothing is left on the stack - change later
                RETURN(RUN_OK);
            }

            case OP_PRINT: {
                //xxx
                printf("OUTPUT: ");
                //xxx

                print_value(POP());
                printf("\n");
                break;
            }

            default: {
                ERROR("Unknown opcode: %d\n", opcode);
            }
        }
    }

    assert(!"Must not reach");
    RETURN(RUN_RUNTIME_ERROR);

    #undef BINARY_NUMBER_OP
    #undef BINARY_FN_OP
    #undef UNARY_NUMBER_OP
    #undef PEEK
    #undef POP
    #undef PUSH
    #undef READ_CONST_LONG
    #undef READ_CONST
    #undef READ_UINT32
    #undef READ_INT16
    #undef READ_BYTE
    #undef READ_TYPE
    #undef ERROR
    #undef RETURN
}

void vm_stack_dump(const vm_t *vm) {
    printf("Stack: [");

    for (const value_t *slot = vm->stack; slot < vm->sp; slot++) {
        print_value(*slot);
        printf(" ");
    }

    printf("] (top)\n");
}

void vm_stack_push(vm_t* vm, value_t value) {
    assert(vm);

    //printf("push %lf\n", value);

    *vm->sp = value;
    vm->sp++;
}

value_t vm_stack_pop(vm_t* vm) {
    assert(vm);

    vm->sp--;
    value_t value = *vm->sp;

    //printf("pop %lf\n", value);

    return value;
}

value_t vm_stack_peek(vm_t* vm, int offset) {
    assert(offset >= 0);
    // sp[0] -> next free slot
    // sp[-1] -> most recent value
    // sp[-2] -> second most recent value
    // ...
    return vm->sp[-1 - offset];
}