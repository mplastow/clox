// clox - vm.h

#ifndef CLOX_VM_H
#define CLOX_VM_H

#include "chunk.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frame_count;

    Value stack[STACK_MAX]; // array of Values, length STACK_MAX, uninitialized by default
    Value* stack_top; // Points to item after last item in stack
    Table globals;
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
