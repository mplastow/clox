// clox - vm.c

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

#include <stdarg.h>
#include <stdio.h>

// Note(matt): should probably static if a global, and having a global is bad anyway!
VM vm;

// Resets the stack by assigning the stack pointer to the first item in the stack
static void resetStack(void)
{
    // Note(matt): relies on stack[] decaying to a pointer
    vm.stack_top = vm.stack;
}

static void runtimeError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}

void initVM(void)
{
    resetStack();
};

void freeVM(void) {};

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

static bool isFalsey(Value value)
{
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

static InterpretResult run(void)
{
// Note(matt): pointer arithmetic? inside a preprocessor directive??
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
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
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
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
        case OP_FALSE: {
            push(BOOL_VAL(0));
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
            BINARY_OP(NUMBER_VAL, +);
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
        case OP_RETURN: {
            printValue(pop());
            printf("\n");
            return INTERPRET_OK;
        } break;
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
}

InterpretResult interpret(const char* source)
{
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(source, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    freeChunk(&chunk);
    return result;
}
