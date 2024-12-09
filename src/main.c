// clox - main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl()
{
    // Buffer to hold line input
    char line[1024];

    // Run the REPL until exit
    for (;;) {
        printf("> ");

        // Read up to sizeof(line) characters from the REPL input
        // Note(matt): fgets() is not unsafe, but care must be taken to use it
        //  safely. However, gets() is UNSAFE and should NEVER BE USED!!
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        interpret(line);
    }
}

static char* readFile(const char* path)
{
    // Get a handle to the file
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file\"%s\".\n", path);
        exit(74);
    }

    // Get file size by seeking to end, getting the offset, then rewinding to the head
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    // Allocate buffer to hold file
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Could not allocate memory to read \"%s\".\n", path);
        exit(74);
    }

    // Read file into buffer
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes_read] = '\0'; // add null terminator

    fclose(file);

    return buffer;
}

static void runFile(const char* path)
{
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);

    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}

int main(int argc, const char* argv[])
{
    // Initialization
    initVM();

    // Parse CLI arguments
    if (argc == 1) {
        repl();
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    // Deinitialization
    freeVM();

    return 0;
}
