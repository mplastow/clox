zig cc \
    src/main.c src/chunk.c src/memory.c src/debug.c src/value.c src/vm.c \
    -g -Iinclude -L/usr/local/gcc-14.2.0/lib64/ -lasan -fsanitize=address -Og \
    -o ./zig-out/bin/asan_out
./zig-out/bin/asan_out
