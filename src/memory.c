// clox - memory.c

#include "memory.h"

#include "compiler.h"
#include "vm.h"

#include <stdlib.h>

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif // DEBUG_LOG_GC

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    vm.bytes_allocated += new_size - old_size;

    if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
        collectGarbage();
#endif // DEBUG_STRESS_GC
        if (vm.bytes_allocated > vm.next_gc) {
            collectGarbage();
        }
    }

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
#ifdef DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)object, object->type);
#endif // DEBUG_LOG_GC

    switch (object->type) {
    case OBJ_CLASS: {
        ObjClass* klass = (ObjClass*)object;
        markObject((Obj*)klass->name);
    } break;
    case OBJ_CLOSURE: {
        ObjClosure* closure = (ObjClosure*)object;
        FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalue_count);
        FREE(ObjClosure, object);
    } break;
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
    case OBJ_UPVALUE: {
        FREE(ObjUpvalue, object);
    } break;
    }
}

void markObject(Obj* object)
{
    if (object == NULL) {
        return;
    }
    if (object->is_marked) {
        return;
    }

#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif // DEBUG_LOG_GC

    object->is_marked = 1;

    if (vm.gray_capacity < vm.gray_count + 1) {
        vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
        vm.gray_stack = (Obj**)realloc(vm.gray_stack, sizeof(Obj*) * vm.gray_capacity);
    }

    if (vm.gray_stack == NULL) {
        exit(1);
    }

    vm.gray_stack[vm.gray_count++] = object;
}

void markValue(Value value)
{
    if (IS_OBJ(value)) {
        markObject(AS_OBJ(value));
    }
}

static void markArray(ValueArray* array)
{
    for (int i = 0; i < array->count; i++) {
        markValue(array->values[i]);
    }
}

static void blackenObject(Obj* object)
{
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    printValue(OBJ_VAL(object));
    printf("\n");
#endif // DEBUG_LOG_GC

    switch (object->type) {
    case OBJ_CLOSURE: {
        ObjClosure* closure = (ObjClosure*)object;
        markObject((Obj*)closure->function);
        for (int i = 0; i < closure->upvalue_count; i++) {
            markObject((Obj*)closure->upvalues[i]);
        }
    } break;
    case OBJ_FUNCTION: {
        ObjFunction* function = (ObjFunction*)object;
        markObject((Obj*)function->name);
        markArray(&function->chunk.constants);
    } break;
    case OBJ_UPVALUE: {
        markValue(((ObjUpvalue*)object)->closed);
    } break;
    case OBJ_NATIVE:
    case OBJ_STRING:
        break;
    }
}

static void markRoots()
{
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
        markValue(*slot);
    }

    for (int i = 0; i < vm.frame_count; i++) {
        markObject((Obj*)vm.frames[i].closure);
    }

    for (ObjUpvalue* upvalue = vm.open_upvalues; upvalue != NULL; upvalue = upvalue->next) {
        markObject((Obj*)upvalue);
    }

    markTable(&vm.globals);
    markCompilerRoots();
}

static void traceReferences()
{
    while (vm.gray_count > 0) {
        Obj* object = vm.gray_stack[--vm.gray_count];
        blackenObject(object);
    }
}

static void sweep()
{
    Obj* previous = NULL;
    Obj* object = vm.objects;
    while (object != NULL) {
        if (object->is_marked) {
            object->is_marked = 0;
            previous = object;
            object = object->next;
        } else {
            Obj* unreached = object;
            object = object->next;
            if (previous != NULL) {
                previous->next = object;
            } else {
                vm.objects = object;
            }

            freeObject(unreached);
        }
    }
}

void collectGarbage()
{
#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.bytes_allocated;
#endif // DEBUG_LOG_GC

    markRoots();
    traceReferences();
    tableRemoveWhite(&vm.strings);
    sweep();

    vm.next_gc = vm.bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");
    printf("   collected %zu bytes (from %zu to %zu) next at %zu\n",
        before - vm.bytes_allocated, before, vm.bytes_allocated,
        vm.next_gc);
#endif // DEBUG_LOG_GC
}

void freeObjects()
{
    Obj* object = vm.objects;
    while (object != NULL) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }

    free(vm.gray_stack);
}
