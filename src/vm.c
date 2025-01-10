// clox - vm.c

#include "vm.h"

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Note(matt): should probably static if a global, and having a global is bad anyway!
VM vm;

static Value clockNative(int arg_count, Value* args)
{
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

// Resets the stack by assigning the stack pointer to the first item in the stack
static void resetStack(void)
{
    // Note(matt): relies on stack[] decaying to a pointer
    vm.stack_top = vm.stack;
    vm.frame_count = 0;
    vm.open_upvalues = NULL;
}

static void runtimeError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frame_count - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - function->chunk.code - 1;
        fprintf(stderr, "[line %d] in ",
            function->chunk.lines[instruction]);
        if (function->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", function->name->chars);
        }
    }
    resetStack();
}

static void defineNative(const char* name, NativeFn function)
{
    push(OBJ_VAL(copyString(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(function)));
    tableSet(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}

void initVM(void)
{
    resetStack();
    vm.objects = NULL;
    initTable(&vm.globals);
    initTable(&vm.strings);

    defineNative("clock", clockNative);
}

void freeVM(void)
{
    freeTable(&vm.globals);
    freeTable(&vm.strings);
    freeObjects();
}

void push(Value value)
{
    // Dereference the stack pointer and place the value there, then increment stack pointer
    *vm.stack_top = value;
    vm.stack_top++;
    // Note(matt): What if we go off the end of the stack???
}

Value pop(void)
{
    // Move the stack pointer back one item, then return the value there
    vm.stack_top--;
    return *vm.stack_top;
}

static Value peek(int distance)
{
    return vm.stack_top[-1 - distance];
}

static bool call(ObjClosure* closure, int arg_count)
{
    if (arg_count != closure->function->arity) {
        runtimeError("Expected %d arguments but got %d.",
            closure->function->arity, arg_count);
        return 0;
    }

    if (vm.frame_count == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return 0;
    }

    CallFrame* frame = &vm.frames[vm.frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.code;
    frame->slots = vm.stack_top - arg_count - 1;
    return 1;
}

static bool callValue(Value callee, int arg_count)
{
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
        case OBJ_CLOSURE: {
            return call(AS_CLOSURE(callee), arg_count);
        }
        case OBJ_NATIVE: {
            NativeFn native = AS_NATIVE(callee);
            Value result = native(arg_count, vm.stack_top - arg_count);
            vm.stack_top -= arg_count + 1;
            push(result);
            return 1;
        }
        default:
            break; // Non-callable object type
        }
    }
    runtimeError("Can only call functions and classes.");
    return 0;
}

static ObjUpvalue* captureUpvalue(Value* local)
{
    ObjUpvalue* prev_upvalue = NULL;
    ObjUpvalue* upvalue = vm.open_upvalues;
    while (upvalue != NULL && upvalue->location > local) {
        prev_upvalue = upvalue;
        upvalue = upvalue->next;
    }

    if (upvalue != NULL && upvalue->location == local) {
        return upvalue;
    }

    ObjUpvalue* created_upvalue = newUpvalue(local);
    created_upvalue->next = upvalue;

    if (prev_upvalue == NULL) {
        vm.open_upvalues = created_upvalue;
    } else {
        prev_upvalue->next = created_upvalue;
    }

    return created_upvalue;
}

static void closeUpvalues(Value* last)
{
    while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
        ObjUpvalue* upvalue = vm.open_upvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm.open_upvalues = upvalue->next;
    }
}

static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static void concatenate()
{
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length = '\0'];

    ObjString* result = takeString(chars, length);
    push(OBJ_VAL(result));
}

static InterpretResult run(void)
{
    CallFrame* frame = &vm.frames[vm.frame_count - 1];
// Note(matt): pointer arithmetic? inside a preprocessor directive??
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op)                          \
    do {                                                  \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
            runtimeError("Operands must be numbers.");    \
            return INTERPRET_RUNTIME_ERROR;               \
        }                                                 \
        double b = AS_NUMBER(pop());                      \
        double a = AS_NUMBER(pop());                      \
        push(valueType(a op b));                          \
    } while (0)

    // Run until we've run out of instructions -- the heart of the VM
    for (;;) {

// Debugging printer
// Note(matt): could be moved to its own function, since `vm` is a global anyway
#ifdef DEBUG_TRACE_EXECUTION
        // Print current contents of stack before interpreting each instruction
        printf("          ");
        for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
            printf("[ ");
            printValue(*slot);
            printf(" ]");
        }
        printf("\n");

        // Disassemble and print each instruction
        disassembleInstruction(&frame->closure->function->chunk,
            (int)(frame->ip - frame->closure->function->chunk.code));
#endif // DEBUG_TRACE_EXECUTION

        // Read, interpret, and execute a single bytecode instruction
        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            push(constant);
        } break;
        case OP_NIL: {
            push(NIL_VAL);
        } break;
        case OP_TRUE: {
            push(BOOL_VAL(1));
        } break;
        case OP_POP: {
            pop();
        } break;
        case OP_GET_LOCAL: {
            uint8_t slot = READ_BYTE();
            push(frame->slots[slot]);
        } break;
        case OP_SET_LOCAL: {
            uint8_t slot = READ_BYTE();
            frame->slots[slot] = peek(0);
        } break;
        case OP_GET_GLOBAL: {
            ObjString* name = READ_STRING();
            Value value;
            if (!tableGet(&vm.globals, name, &value)) {
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
            push(value);
        } break;
        case OP_DEFINE_GLOBAL: {
            ObjString* name = READ_STRING();
            tableSet(&vm.globals, name, peek(0));
            pop();
        } break;
        case OP_FALSE: {
            push(BOOL_VAL(0));
        } break;
        case OP_SET_GLOBAL: {
            ObjString* name = READ_STRING();
            if (tableSet(&vm.globals, name, peek(0))) {
                tableDelete(&vm.globals, name);
                runtimeError("Undefined variable '%s'.", name->chars);
                return INTERPRET_RUNTIME_ERROR;
            }
        } break;
        case OP_GET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            push(*frame->closure->upvalues[slot]->location);
        } break;
        case OP_SET_UPVALUE: {
            uint8_t slot = READ_BYTE();
            *frame->closure->upvalues[slot]->location = peek(0);
        } break;
        case OP_EQUAL: {
            Value b = pop();
            Value a = pop();
            push(BOOL_VAL(valuesEqual(a, b)));
        } break;
        case OP_GREATER: {
            BINARY_OP(BOOL_VAL, >);
        } break;
        case OP_LESS: {
            BINARY_OP(BOOL_VAL, <);
        } break;
        case OP_ADD: {
            if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                concatenate();
            } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                double b = AS_NUMBER(pop());
                double a = AS_NUMBER(pop());
                push(NUMBER_VAL(a + b));
            } else {
                runtimeError("Operands must be two numbers or two strings.");
                return INTERPRET_RUNTIME_ERROR;
            }
        } break;
        case OP_SUBTRACT: {
            BINARY_OP(NUMBER_VAL, -);
        } break;
        case OP_MULTIPLY: {
            BINARY_OP(NUMBER_VAL, *);
        } break;
        case OP_DIVIDE: {
            BINARY_OP(NUMBER_VAL, /);
        } break;
        case OP_NOT: {
            push(BOOL_VAL(isFalsey(pop())));
        } break;
        case OP_NEGATE: {
            // Note(matt): elegant solution to negating top Value in stack
            if (!IS_NUMBER(peek(0))) {
                runtimeError("Operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
        } break;
        case OP_PRINT: {
            printValue(pop());
            printf("\n");
        } break;
        case OP_JUMP: {
            uint16_t offset = READ_SHORT();
            frame->ip += offset;
        } break;
        case OP_JUMP_IF_FALSE: {
            uint16_t offset = READ_SHORT();
            if (isFalsey(peek(0))) {
                frame->ip += offset;
            }
        } break;
        case OP_LOOP: {
            uint16_t offset = READ_SHORT();
            frame->ip -= offset;
        } break;
        case OP_CALL: {
            int arg_count = READ_BYTE();
            if (!callValue(peek(arg_count), arg_count)) {
                return INTERPRET_RUNTIME_ERROR;
            }
            frame = &vm.frames[vm.frame_count - 1];
        } break;
        case OP_CLOSURE: {
            ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
            ObjClosure* closure = newClosure(function);
            push(OBJ_VAL(closure));
            for (int i = 0; i < closure->upvalue_count; i++) {
                uint8_t is_local = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (is_local) {
                    closure->upvalues[i] = captureUpvalue(frame->slots + index);
                } else {
                    closure->upvalues[i] = frame->closure->upvalues[index];
                }
            }
        } break;
        case OP_CLOSE_UPVALUE: {
            closeUpvalues(vm.stack_top - 1);
            pop();
        } break;
        case OP_RETURN: {
            Value result = pop();
            closeUpvalues(frame->slots);
            vm.frame_count--;
            if (vm.frame_count == 0) {
                pop();
                // Exit interpreter
                return INTERPRET_OK;
            }

            vm.stack_top = frame->slots;
            push(result);
            frame = &vm.frames[vm.frame_count - 1];
        } break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

InterpretResult interpret(const char* source)
{
    ObjFunction* function = compile(source);
    if (function == NULL) {
        return INTERPRET_COMPILE_ERROR;
    }

    push(OBJ_VAL(function));
    ObjClosure* closure = newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    call(closure, 0);

    return run();
}
