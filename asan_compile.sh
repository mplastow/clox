zig cc \
    src/main.c \
    src/chunk.c \
    src/compiler.c \
    src/debug.c \
    src/memory.c \
    src/object.c \
    src/scanner.c \
    src/table.c \ 
    src/value.c \
    src/vm.c \
    -g -Og -Iinclude -L/usr/local/gcc-14.2.0/lib64/ -lasan -fsanitize=address \
    -o ./zig-out/bin/asan_out
./zig-out/bin/asan_out
