// clox - value.c

#include "value.h"
#include "object.h"
#include "memory.h"

#include <stdio.h>
#include <string.h>

void initValueArray(ValueArray* array)
{
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray* array, Value value)
{
    // Grow the current array if it does not have capacity for the new byte
    if (array->capacity < (array->count + 1)) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
    }

    // Append the value to the chunk and increment the count of values
    array->values[array->count] = value;
    array->count++;
}

void freeValueArray(ValueArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    initValueArray(array);
}

void printValue(Value value)
{
    switch (value.type) {
    case VAL_BOOL: {
        printf(AS_BOOL(value) ? "true" : "false");
    } break;
    case VAL_NIL: {
        printf("nil");
    } break;
    case VAL_NUMBER: {
        printf("%g", AS_NUMBER(value));
    } break;
    case VAL_OBJ: {
        printObject(value);
    } break;
    }
}

bool valuesEqual(Value a, Value b)
{
    if (a.type != b.type) {
        return 0;
    }

    switch (a.type) {
    case VAL_BOOL: {
        return AS_BOOL(a) == AS_BOOL(b);
    }
    case VAL_NIL: {
        return 1;
    }
    case VAL_NUMBER: {
        return AS_NUMBER(a) == AS_NUMBER(b);
    }
    case VAL_OBJ: {
        ObjString* aString = AS_STRING(a);
        ObjString* bString = AS_STRING(b);
        return aString->length == bString->length
            && memcmp(aString->chars, bString->chars, aString->length) == 0;
    } break;
    default: {
        return 0; // Unreachable
    }
    }
}
