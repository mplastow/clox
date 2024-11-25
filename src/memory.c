// clox - memory.c

#include "memory.h"

#include <stdlib.h>

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    // TODO(matt): Cover all cases for old_size, new_size
    //              - Both 0
    //              - Both nonzero but same size

    if (new_size == 0) {
        free(ptr);
        return NULL;
    }

    // Handles cases where new_size > 0
    void* result = realloc(ptr, new_size);
    // Handles allocation failure due to OOM
    if (result == NULL)
        exit(1);
    return result;
}
