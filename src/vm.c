// clox - vm.c

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

#include <stdio.h>

// Note(matt): should probably static if a global, and having a global is bad anyway!
VM vm;

// Resets the stack by assigning the stack pointer to the first item in the stack
static void resetStack(void)
{
    // Note(matt): relies on stack[] decaying to a pointer
    vm.stack_top = vm.stack;
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

static InterpretResult run(void)
{
// Note(matt): pointer arithmetic? inside a preprocessor directive??
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define BINARY_OP(op)     \
    do {                  \
        double b = pop(); \
        double a = pop(); \
        push(a op b);     \
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
        case OP_ADD:
            BINARY_OP(+);
            break;
        case OP_SUBTRACT:
            BINARY_OP(-);
            break;
        case OP_MULTIPLY:
            BINARY_OP(*);
            break;
        case OP_DIVIDE:
            BINARY_OP(/);
            break;
        case OP_NEGATE: {
            // Note(matt): elegant solution to negating top Value in stack
            push(-pop());
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
