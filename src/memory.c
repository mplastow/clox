// clox - memory.c

#include "memory.h"

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
