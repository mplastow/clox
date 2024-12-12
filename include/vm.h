// clox - vm.h

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    uint8_t* ip;
    Value stack[STACK_MAX]; // array of Values, length STACK_MAX, uninitialized by default
    Value* stack_top; // Points to item after last item in stack
    Table strings;
    Obj* objects;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

// Initializes the Lox virtual machine
void initVM(void);
// Deinitializes the Lox virtual machine
void freeVM(void);
// Interprets the bytecode instructions inside the Lox virtual machine
//  90% of the BM runtime is spent inside this function, via the call to run()
InterpretResult interpret(const char* source);
// Push a Value onto the instruction stack
void push(Value value);
// Pop the last value on the instruction stack
Value pop(void);

#endif // CLOX_VM_H
