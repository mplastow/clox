// clox - debug.h

#ifndef CLOX_DEBUG_H
#define CLOX_DEBUG_H

#include "chunk.h"

// Disassembles all instructions in a chunk
void disassembleChunk(Chunk* chunk, const char* name);
// Disassembles a single instruction at an offset into a chunk
int disassembleInstruction(Chunk* chunk, int offset);

#endif // CLOX_DEBUG_H
