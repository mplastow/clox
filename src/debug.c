// clox - debug.c

#include "debug.h"
#include "value.h"

#include <stdio.h>

void disassembleChunk(Chunk* chunk, const char* name)
{
    printf("== %s ==\n", name);

    // Note(matt): the increment to the loop variable is _inside_ the function
    //  body and is hidden behind the function call!!
    for (int offset = 0; offset < chunk->count;) {
        offset = disassembleInstruction(chunk, offset);
    }
}

// Prints an instruction and increments the offset
static int simpleInstruction(const char* name, int offset)
{
    printf("%s\n", name);

    return offset + 1;
}

// Prints a constant instruction and its associated value, then increments the offset
static int constantInstruction(const char* name, Chunk* chunk, int offset)
{
    uint8_t constant = chunk->code[offset + 1];
    printf("%-16s %4d '", name, constant);
    printValue(chunk->constants.values[constant]);
    printf("'\n");
    return offset + 2;
}

int disassembleInstruction(Chunk* chunk, int offset)
{
    // Print offset into chunk as a signpost
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("%4d ", chunk->lines[offset]);
    }

    // Read a single-byte opcode from the bytecode stream
    uint8_t instruction = chunk->code[offset];

    // Display the opcode using a utility function
    switch (instruction) {
    case OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OP_ADD:
        return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
        return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
        return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
        return simpleInstruction("OP_DIVIDE", offset);
    case OP_NEGATE:
        return simpleInstruction("OP_NEGATE", offset);
    case OP_RETURN:
        return simpleInstruction("OP_RETURN", offset);

    // If the opcode is not recognized...
    default:
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
}
