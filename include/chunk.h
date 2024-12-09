// clox - chunk.h

#ifndef CLOX_CHUNK_H
#define CLOX_CHUNK_H

#include "common.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValueArray constants;
} Chunk;

// Chunk memory management functions
// Initializes a new chunk by setting the count and capacity to 0
void initChunk(Chunk* chunk);
// Appends a single byte to the end of a chunk
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addConstant(Chunk* chunk, Value value);
// Frees the memory associated with a chunk and sets count and capacity to 0
void freeChunk(Chunk* chunk);

#endif // CLOX_CHUNK_H
