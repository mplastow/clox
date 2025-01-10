// clox - memory.h

#ifndef CLOX_MEMORY_H
#define CLOX_MEMORY_H

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

// Calculates a new capacity based on a given current capacity
//  Growth factor is 2
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// Grows an array to a specified size by calling `reallocate()`
#define GROW_ARRAY(type, ptr, old_count, new_count)    \
    (type*)reallocate(ptr, sizeof(type) * (old_count), \
        sizeof(type) * (new_count))

// Frees all memory associated with an array
#define FREE_ARRAY(type, ptr, old_count) \
    reallocate(ptr, sizeof(type) * (old_count), 0)

// Allocates and reallocates blocks of memory, controlled by inputs.
//  `reallocate()` is used for all dynamic memory management in clox.
//
//  Chart of memory operations:
//  -------------------------------------------------------------
//  | old_size      new_size        operation                   |
//  |-----------------------------------------------------------|
//  | 0             > 0             Allocate new block          |
//  | > 0           0               Free allocation             |
//  | > 0           < old_size      Shrink existing allocation  |
//  | > 0           > old_size      Grow existing allocation    |
//  -------------------------------------------------------------
//
//  Note(matt): `void*` indicates a type-erased pointer below and should be avoided
void* reallocate(void* ptr, size_t old_size, size_t new_size);
void markObject(Obj* object);
void markValue(Value value);
void collectGarbage();
void freeObjects();

#endif // CLOX_MEMORY_H
