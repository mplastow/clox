// clox - memory.c

#include "memory.h"

#include "vm.h"

#include <stdlib.h>

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    // If new_size is not larger than old_size, return a null pointer
    if (new_size == 0) {
        free(ptr);
        return NULL;
    } else if (new_size <= old_size) {
        free(ptr);
        return NULL;
    }

    // If new_size > 0, allocate some more memory
    void* result = realloc(ptr, new_size);
    // Handles allocation failure due to OOM
    if (result == NULL)
        exit(1);
    return result;
}

static void freeObject(Obj* object)
{
    switch (object->type) {
    case OBJ_FUNCTION: {
        ObjFunction* function = (ObjFunction*)object;
        freeChunk(&function->chunk);
        FREE(ObjFunction, object);
    } break;
    case OBJ_NATIVE: {
        FREE(ObjNative, object);
    } break;
    case OBJ_STRING: {
        ObjString* string = (ObjString*)object;
        FREE_ARRAY(char, string->chars, string->length + 1);
        FREE(ObjString, object);
    } break;
    }
}

void freeObjects()
{
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}
