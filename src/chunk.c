// clox - chunk.c

#include "chunk.h"
#include "memory.h"

#include <stdlib.h>

void initChunk(Chunk* chunk)
{
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants); // Note(matt): Reference operator & is a bad idea
}

void writeChunk(Chunk* chunk, uint8_t byte, int line)
{
    // Grow the current array if it does not have capacity for the new byte
    if (chunk->capacity < (chunk->count + 1)) {
        int old_capacity = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old_capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_capacity, chunk->capacity);
        chunk->lines = GROW_ARRAY(uint8_t, chunk->lines, old_capacity, chunk->capacity);
    }

    // Append the byte to the chunk and increment the count of bytes
    chunk->code[chunk->count] = byte;
    chunk->lines[chunk->count] = line;
    chunk->count++;
}

int addConstant(Chunk* chunk, Value value)
{
    writeValueArray(&chunk->constants, value); // Note(matt): Reference operator & is a bad idea
    return chunk->constants.count - 1;
}

void freeChunk(Chunk* chunk)
{
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    initChunk(chunk);
}
