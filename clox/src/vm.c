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
#include <time.h>

//#define VM_TRACE_EXECUTION

#define VM_FRAMES_MAX   256

#define VM_STACK_MAX    (VM_FRAMES_MAX * UINT8_MAX)



typedef struct {
    const closure_object_t* closure;
    const uint8_t* ip;
    value_t* base_pointer;
    // [0] = closure object
    // [1] = first local
    // [2] = second local
    // ...
} call_frame_t;

typedef struct vm {
    call_frame_t frames[VM_FRAMES_MAX];
    size_t frame_count;

    value_t stack[VM_STACK_MAX];
    value_t* sp;

    object_root_t root;
    table_t globals;

    bool has_runtime_error;
} vm_t;



static void runtime_error(vm_t* vm, const char* format, ...);



static void register_native(vm_t* vm, const char* name, size_t arity, native_fn_t fn) {
    assert(vm);
    assert(name);
    assert(fn);

    size_t name_len = strlen(name);
    assert(name_len);

    // Note: stored on stack to prevent cleanup by GC.
    vm_stack_push(vm, OBJECT_VALUE((object_t*)create_string_object(&vm->root, name, name_len)));
    vm_stack_push(vm, OBJECT_VALUE((object_t*)create_native_object(&vm->root, name, arity, fn)));

    assert(IS_STRING(vm->sp[-2]));
    assert(IS_NATIVE(vm->sp[-1]));

    const bool r = table_set(&vm->globals, vm->sp[-2], vm->sp[-1]);
    (void)r;
    assert(r);

    vm_stack_pop(vm);
    vm_stack_pop(vm);
}

static bool native_clock(void* context, size_t arg_count, const value_t* args, value_t* result) {
    (void)context;
    (void)arg_count;
    (void)args;

    const double t = (double)clock() / CLOCKS_PER_SEC;
    *result = NUMBER_VALUE(t);
    
    return true;
}

static bool native_dump(void* context, size_t arg_count, const value_t* args, value_t* result) {
    (void)context;
    (void)result;

    printf("native_dump(%zu args):\n", arg_count);
    for(size_t i=0; i<arg_count; i++) {
        printf("arg[%zu] = ", i);
        print_value(args[i]);
        printf("\n");
    }

    return true;
}

static bool native_print(void* context, size_t arg_count, const value_t* args, value_t* result) {
    (void)context;
    (void)result;

    // TODO implement format strings etc
    
    for(size_t i=0; i<arg_count; i++) {
        print_value(args[i]);
    }
    printf("\n");

    return true;
}

static bool native_tostring(void* context, size_t arg_count, const value_t* args, value_t* result) {
    (void)arg_count;
    (void)result;

    vm_t* const vm = (vm_t*)context;
    const value_t value = args[0];

    char buffer[128];
    print_value_to_buffer(buffer, sizeof(buffer), value);
    *result = OBJECT_VALUE((object_t*)create_string_object(&vm->root, buffer, strlen(buffer)));

    return true;
}

static bool native_assert(void* context, size_t arg_count, const value_t* args, value_t* result) {
    (void)arg_count;
    (void)result;

    vm_t* const vm = (vm_t*)context;

    const value_t value = args[0];
    if (!IS_BOOL(value)) {
        runtime_error(vm, "Invalid value type (%d)", (int)value.type);
        return false;
    }

    const bool bool_value = AS_BOOL(value);
    if (!bool_value) {
        // TODO somehow show exact expression which failed?
        runtime_error(vm, "Assertion failed");
        return false;
    }

    // all good
    return true;
}



vm_t* vm_create(void) {
    vm_t *vm = (vm_t*)malloc(sizeof(vm_t));
    assert(vm);

    memset(vm, 0, sizeof(vm_t));

    vm->sp = vm->stack; // sp points to next free slot

    object_root_init(&vm->root);
    table_init(&vm->globals);

    register_native(vm, "clock", 0, native_clock);
    register_native(vm, "dump", SIZE_MAX, native_dump);
    register_native(vm, "printf", SIZE_MAX, native_print); // TODO there is some kind of collision with the print-statement if it's just called "print"
    register_native(vm, "tostring", 1, native_tostring);
    register_native(vm, "assert", 1, native_assert);

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
    // TODO should call this from vm_create() ?
}

static call_frame_t* get_current_frame(vm_t* vm) {
    assert(vm);
    assert(vm->frame_count > 0);

    return vm->frames + vm->frame_count - 1;
}

[[maybe_unused]]
static const chunk_t* get_current_chunk(vm_t* vm) {
    const call_frame_t* const frame = get_current_frame(vm);

    assert(frame);
    assert(frame->closure);
    assert(frame->closure->function);

    return &frame->closure->function->chunk;
}

static void runtime_error(vm_t* vm, const char* format, ...) {
    // print error
    {
        fprintf(stderr, "RuntimeError: ");
     
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);

        size_t len = strlen(format);
        if (len > 0 && format[len-1] != '.') {
            fputs(".", stderr);
        }

        fputs("\n", stderr);
    }

    // print call stack
    {
        for (size_t frame_index = vm->frame_count - 1 ; ; ) {
            const call_frame_t* const frame = vm->frames + frame_index;
            const function_object_t* const function = frame->closure->function;
            const chunk_t* const chunk = &function->chunk;
            const size_t offset = frame->ip - chunk->code - 1;
            const uint32_t line = chunk_get_line_for_offset(chunk, offset);

            if (function->name) {
                fprintf(stderr, "[line %u] in %s()\n", line, function->name->chars);
            } else {
                fprintf(stderr, "[line %u] in script\n", line);
            }

            if (frame_index == 0) break;
            frame_index--;
        }
    }

    reset_stack(vm);

    vm->has_runtime_error = true;
}

static closure_object_t* create_closure(vm_t* vm, const function_object_t* function);
static bool call(vm_t* vm, value_t callee, size_t arg_count);
static run_result_t vm_run(vm_t* vm);

run_result_t vm_run_source(vm_t* vm, const char* source) {
    // compile source to function-object
    const function_object_t* const function = compile(&vm->root, source);
    if (!function) {
        return RUN_COMPILE_ERROR;
    }

    vm_stack_push(vm, OBJECT_VALUE((object_t*)function)); // prevent GC

    // create closure-object from function-object
    const closure_object_t* const closure = create_closure(vm, function);

    vm_stack_pop(vm);
    vm_stack_push(vm, OBJECT_VALUE((object_t*)closure)); // local 0

    // create initial call frame
    call(vm, OBJECT_VALUE((object_t*)closure), 0);

    // run
    const run_result_t result = vm_run(vm);

    // vm_run should return a clean vm
    if (result == RUN_OK) {
        assert(vm->sp == vm->stack);
        assert(vm->frame_count == 0);
    } else {
        // TODO ?
    }

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

static closure_object_t* create_closure(vm_t* vm, const function_object_t* function) {
    assert(function);

    closure_object_t* closure = create_closure_object(&vm->root, function);

    return closure;
}

static upvalue_object_t* capture_upvalue(vm_t* vm, value_t* target) {
    assert(target);

    upvalue_object_t* current = vm->root.open_upvalues;
    upvalue_object_t* previous = NULL;

    // The list starts at the highest target-address - the last entry has the lowest target-address.
    // Skip all open upvalues with target at higher address
    while (current && current->target > target) {
        previous = current;
        current = current->next;
    }

    // Found perfect match?
    if (current && current->target == target) {
        return current;
    }

    // Create new upvalue-object.
    upvalue_object_t* const new_upvalue = create_upvalue_object(&vm->root, target);

    // Insert to list.
    if (previous == NULL) {
        vm->root.open_upvalues = new_upvalue;
    } else {
        previous->next = new_upvalue;
    }
    new_upvalue->next = current; // has lower target-address than new upvalue / or NULL

    return new_upvalue;
}

static void close_upvalue(vm_t* vm, value_t* target) {
    assert(target);

    // list starts at highest target-address - has item has lowest target-address.
    // close all upvalues with higher or equal target-address than the passed address.

    while (vm->root.open_upvalues && 
           vm->root.open_upvalues->target >= target) {

        upvalue_object_t* const upvalue = vm->root.open_upvalues;

        // close
        upvalue->closed = *upvalue->target;
        upvalue->target = &upvalue->closed;

        // remove from list
        vm->root.open_upvalues = upvalue->next;
    }
}

static bool call(vm_t* vm, value_t callee, size_t arg_count) {

    //xxx
    // printf(">>> call: ");
    // print_value(callee);
    // printf(" arg_count=%zu\n", arg_count);
    //xxx

    if (IS_OBJECT(callee)) {
        switch (OBJECT_TYPE(callee)) {

            case OBJECT_TYPE_CLOSURE: {
                const closure_object_t* const closure = AS_CLOSURE(callee);
                const function_object_t* const function = closure->function;

                if (function->arity != arg_count) {
                    runtime_error(vm, "Expected %zu arguments but got %zu.", function->arity, arg_count);
                    return false;
                }

                if (vm->frame_count >= VM_FRAMES_MAX) {
                    runtime_error(vm, "Call stack overflow.");
                    return false;
                }

                vm->frame_count++;
                call_frame_t* const frame = vm->frames + vm->frame_count - 1;

                frame->closure = closure;
                frame->ip = function->chunk.code;
                frame->base_pointer = vm->sp - arg_count - 1; // point to: [closure-obj] [arg1] [arg2] ...

                assert(IS_CLOSURE(frame->base_pointer[0]));

                return true;
            }

            case OBJECT_TYPE_NATIVE: {
                const native_object_t* const native = AS_NATIVE(callee);

                if (native->arity != SIZE_MAX && native->arity != arg_count) {
                    runtime_error(vm, "Native function '%s': Expected %zu arguments but got %zu.", native->name, native->arity, arg_count);
                    return false;
                }

                value_t result = NIL_VALUE();

                if (!native->fn(vm, arg_count, vm->sp - arg_count, &result)) {
                    if (!vm->has_runtime_error) {
                        runtime_error(vm, "Call to native function '%s' failed", native->name);
                    }
                    return false;
                }

                // drop closure-obj and args
                vm->sp -= arg_count + 1;

                // leave return value on stack
                vm_stack_push(vm, result);

                return true;
            }

            default: {
                break;
            }
        }
    }

    runtime_error(vm, "Can only call functions and classes.");
    return false;
}

#ifndef NDEBUG
static void vm_check_ip_bounds(vm_t* vm, size_t bytes_to_read) {
    const call_frame_t* frame = get_current_frame(vm);
    const chunk_t* chunk = get_current_chunk(vm);

    const uint8_t* const start = chunk->code;
    const uint8_t* const end = chunk->code + chunk->count;

    if (frame->ip < start) {
        printf("Error: IP before chunk start: start=%p end=%p ip=%p bytes_to_read=%zu",
            (void*)start, (void*)end, (void*)frame->ip, bytes_to_read);
        assert(!"IP before chunk start");
    }

    if (frame->ip + bytes_to_read > end) {
        printf("Error: IP after chunk end. start=%p end=%p ip=%p bytes_to_read=%zu",
            (void*)start, (void*)end, (void*)frame->ip, bytes_to_read);
        assert(!"IP after chunk end");
    }
}

static void vm_check_sp_bounds(const vm_t* vm, const value_t* location) {
    const value_t* const start = vm->stack;
    const value_t* const end = vm->stack + VM_STACK_MAX;

    if (location < start) {
        printf("Error: Trying to read before stack start: start=%p end=%p location=%p\n",
            (void*)start, (void*)end, (void*)location);
        assert(!"Trying to read before stack start");
    }

    if (location >= end) {
        printf("Error: Trying to read after stack end: start=%p end=%p location=%p\n",
            (void*)start, (void*)end, (void*)location);
        assert(!"Trying to read after stack end");
    }
}
#endif

static run_result_t vm_run(vm_t* vm) {
    assert(vm);
    assert(vm->frame_count > 0);

    // Note: storing commonly used pointers in local variables for faster access, must be manually updated (OP_CALL, OP_RETURN).
    call_frame_t* frame = vm->frames + vm->frame_count - 1;
    const value_t* values = frame->closure->function->chunk.values.values;
    register const uint8_t* ip = frame->ip;

    assert(frame->closure);
    assert(frame->closure->function);
    assert(frame->ip);
    assert(frame->base_pointer);

    #define ERROR(args...) do { \
        frame->ip = ip; \
        runtime_error(vm, args); \
        return RUN_RUNTIME_ERROR; \
    } while (false)

    #ifndef NDEBUG
    #define CHECK_IP_BOUNDS(read_size) vm_check_ip_bounds(vm, read_size)
    #else
    #define CHECK_IP_BOUNDS(read_size) ((void)0)
    #endif

    #ifndef NDEBUG
    #define CHECK_SP_BOUNDS(loc) vm_check_sp_bounds(vm, loc)
    #else
    #define CHECK_SP_BOUNDS(loc) ((void)0)
    #endif

    // memcpy to prevent unaligned reads (works on x86 but is UB according to C).
    // using feature "statement expression", works on gcc/clang but not on some others like msvc.
    #define READ_TYPE(type) ({ \
        const size_t _size = sizeof(type); \
        CHECK_IP_BOUNDS(_size); \
        type _val; \
        memcpy(&_val, ip, _size); \
        ip += _size; \
        _val; \
    })

    // does not appear to make a difference, leaving it here for laters testing/performance improvements
    //#define READ_INT16() (ip += 2, (int16_t)((ip[-1] << 8) | ip[-2]))
    //#define READ_UINT32() (ip += 4, (uint32_t)((ip[-1] << 8) | (ip[-2] << 8) | (ip[-3] << 8) | ip[-4]))

    #define READ_INT16()    READ_TYPE(int16_t)
    #define READ_UINT32()   READ_TYPE(uint32_t)
    
    #ifndef NDEBUG
    #define READ_BYTE()     READ_TYPE(uint8_t)
    #else
    #define READ_BYTE()     (*(ip++))
    #endif

    // returns value_t
    #define READ_CONST()        (values[READ_BYTE()])
    #define READ_CONST_LONG()   (values[READ_UINT32()])

    // take/return value_t
    #define PUSH(value)         vm_stack_push(vm, value)
    #define POP()               vm_stack_pop(vm)
    #define PEEK(offset)        vm_stack_peek(vm, offset)

    #define UNARY_NUMBER_OP(op) \
        do { \
            if (!IS_NUMBER(PEEK(0))) { \
                ERROR("Operand must be a number"); \
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
            if (!IS_NUMBER(PEEK(0)) || !IS_NUMBER(PEEK(1))) { \
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

            const chunk_t* const chunk = &frame->closure->function->chunk;
            disassemble_instruction(chunk, (size_t)(ip - chunk->code));
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
                
                CHECK_SP_BOUNDS(frame->base_pointer + stack_index);

                const value_t value = frame->base_pointer[stack_index];
                PUSH(value);
                break;
            }

            case OP_SET_LOCAL:
            case OP_SET_LOCAL_LONG: {
                const uint32_t stack_index = opcode == OP_SET_LOCAL ? READ_BYTE() : READ_UINT32();

                CHECK_SP_BOUNDS(frame->base_pointer + stack_index);

                const value_t value = PEEK(0); // leave on stack
                frame->base_pointer[stack_index] = value;
                break;
            }

            case OP_GET_UPVALUE:
            case OP_GET_UPVALUE_LONG: {
                const uint8_t upvalue_index = opcode == OP_GET_UPVALUE ? READ_BYTE() : READ_UINT32();
                assert(upvalue_index < frame->closure->upvalue_count);

                const value_t value = *(frame->closure->upvalues[upvalue_index]->target);
                PUSH(value);
                break;
            }

            case OP_SET_UPVALUE:
            case OP_SET_UPVALUE_LONG: {
                const uint8_t upvalue_index = opcode == OP_SET_UPVALUE ? READ_BYTE() : READ_UINT32();
                assert(upvalue_index < frame->closure->upvalue_count);

                *(frame->closure->upvalues[upvalue_index]->target) = PEEK(0);
                break;
            }

            case OP_JUMP: {
                const int16_t offset = READ_INT16();
                ip += offset;
                break;
            }
            case OP_JUMP_IF_TRUE: {
                const int16_t offset = READ_INT16();
                if (value_is_truey(PEEK(0))) { // leave on stack
                    ip += offset;
                }
                break;
            }
            case OP_JUMP_IF_FALSE: {
                const int16_t offset = READ_INT16();
                if (value_is_falsey(PEEK(0))) { // leave on stack
                    ip += offset;
                }
                break;
            }

            case OP_POP: POP(); break;

            case OP_CALL: {
                // Stack: ... closure-obj arg1 arg2 arg3

                const size_t arg_count = READ_BYTE();
                const value_t callee = PEEK(arg_count);

                frame->ip = ip;

                if (!call(vm, callee, arg_count)) {
                    return RUN_RUNTIME_ERROR;
                }

                // refresh cached current frame
                frame = vm->frames + vm->frame_count - 1;
                values = frame->closure->function->chunk.values.values;
                ip = frame->ip;
                break;
            }

            case OP_RETURN: {
                // Stack before: ... closure-obj arg1 arg2 arg3 ... return-value
                // Stack after : ... return-value

                const value_t return_value = POP();

                // close all open upvalues in the frame we are dropping
                close_upvalue(vm, frame->base_pointer);

                // drop top frame
                vm->frame_count--;

                // exit vm?
                if (vm->frame_count == 0) {
                    POP(); // drop closure-obj
                    return RUN_OK;
                }

                // restore stack to the state before the call
                vm->sp = frame->base_pointer;

                PUSH(return_value);

                // refresh cached current frame
                frame = vm->frames + vm->frame_count - 1;
                values = frame->closure->function->chunk.values.values;
                ip = frame->ip;

                break;
            }

            case OP_CLOSURE: {
                // get function from value-array
                const value_t function_value = READ_CONST();
                assert(IS_FUNCTION(function_value));
                const function_object_t* const function = AS_FUNCTION(function_value);

                // create closure-object
                closure_object_t* const closure = create_closure(vm, function);
                PUSH(OBJECT_VALUE((object_t*)closure));

                // ...
                for (size_t i=0; i<function->upvalue_count; i++) {
                    const uint8_t is_local = READ_BYTE();
                    const uint8_t index = READ_BYTE();

                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(vm, frame->base_pointer + index);
                    } else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }

                break;
            }

            case OP_CLOSE_UPVALUE: {
                close_upvalue(vm, vm->sp - 1);
                POP();
                break;
            }

            case OP_PRINT: {
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
    return RUN_RUNTIME_ERROR;

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

    if (vm->sp == vm->stack + VM_STACK_MAX) {
        printf("Error: Can't push to stack, stack is full!");
        assert(!"Error: Can't push to stack, stack is full!");
    }

    *vm->sp = value;
    vm->sp++;
}

value_t vm_stack_pop(vm_t* vm) {
    assert(vm);

    if (vm->sp == vm->stack) {
        printf("Error: Can't pop from stack, stack is empty!");
        assert(!"Error: Can't pop from stack, stack is empty!");
    }

    vm->sp--;
    value_t value = *vm->sp;

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