// clox - value.h

#ifndef CLOX_VALUE_H
#define CLOX_VALUE_H

#include "common.h"

// In the Lox language, all numbers use floating-point representation (for now)
typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

// ValueArray memory management functions
// Initializes a new ValueArray by setting the count and capacity to 0
void initValueArray(ValueArray* array);
// Appends a single value to the end of a ValueArray
void writeValueArray(ValueArray* array, Value value);
// Frees the memory associated with a ValueArray and sets count and capacity to 0
void freeValueArray(ValueArray* array);
// Prints a value
void printValue(Value value);

#endif // CLOX_VALUE_H
