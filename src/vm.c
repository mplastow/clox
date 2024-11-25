// clox - vm.c

#include "common.h"
#include "debug.h"
#include "vm.h"

#include <stdio.h>

// Note(matt): should probably static if a global, and having a global is bad anyway!
VM vm;

void initVM() {};

void freeVM() {};

static InterpretResult run()
{
// Note(matt): pointer arithmetic? inside a preprocessor directive??
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])

    // Run until we've run out of instructions: heart of the VM
    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        // Disassemble and print each instruction
        disassembleInstruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif // DEBUG_TRACE_EXECUTION
       // Read, interpret, and execute a single bytecode instruction
        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
        case OP_CONSTANT: {
            Value constant = READ_CONSTANT();
            printValue(constant);
            printf("\n");
        } break;
        case OP_RETURN: {
            return INTERPRET_OK;
        }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(Chunk* chunk)
{
    // Store the chunk being executed in the VM
    vm.chunk = chunk;
    // Set the instruction pointer to the first byte of code in the chunk
    vm.ip = vm.chunk->code;

    // Call a helper function to run the bytecode instructions and return result
    return run();
}
