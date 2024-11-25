// clox - vm.h

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

// Initializes the Lox virtual machine
void initVM();
// Deinitializes the Lox virtual machine
void freeVM();
// Interprets the bytecode instructions inside the Lox virtual machine
//  90% of the BM runtime is spent inside this function, via the call to run()
InterpretResult interpret(Chunk* chunk);

#endif // CLOX_VM_H
