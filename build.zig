const std = @import("std");

pub fn build(b: *std.Build) void {

    ////////////////////////////////////////////////////////////////////////////
    // Set project options
    //
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{
        .preferred_optimize_mode = .Debug,
    });

    ////////////////////////////////////////////////////////////////////////////
    // Build executable
    //
    const exe = b.addExecutable(.{
        .name = "prog",
        .target = target,
        .optimize = optimize,
    });

    // Array of .c and .cpp filenames to pass to addCSourceFiles()
    // #includes from any files listed here do not need to be added to this array
    const exe_files = [_][]const u8{
        "src/main.c",
        "src/chunk.c",
        "src/memory.c",
        "src/debug.c",
        "src/value.c",
        "src/vm.c",
    };
    // Array of clang compiler flags to pass to addCSourceFiles()
    const exe_flags = [_][]const u8{
        // Standards and optimization flags
        "-std=c23", // C 23
        "-ggdb3", // Maximizes debug information
        "-Og", // Optimization level: g = debug, s = size, 3 = maximum

        // Warning flags
        "-Wall", // Reasonable default warnings
        "-Wextra", // More reasonable default warnings
        "-Wpedantic", // Enforces ISO C and C++, ensures portability
        // "-Weverything",// EVERY SINGLE WARNING, introduces conflicts, do not use
        "-Wcast-qual", // Warns when type qualifiers are cast away
        "-Wcast-align", // Warns when casts increase required alignment
        "-Wconversion", // Warns on implicit conversion that may result in data loss
        "-Wfloat-equal", // Warns when floating point types are compared for in/equality
        "-Wmismatched-tags", // Warns against previous declarations
        "-Wshadow-all", // Warns when a declaration shadows anything else
        "-Wstrict-prototypes", // Warns on f() rather than f(void)
        "-Wvla", // Warns when using variable-length arrays
        "-Wsign-conversion", // Warns on sign conversion
        "-Wunused", // Warns on unused variables, functions etc.
        "-Wunused-parameter", // Warns on unused parameters
        "-Wunused-template", // Warns on unused templates
        "-Wnon-virtual-dtor", // = "-Weffc++", class has virtual functions but non-virtual destructor

        // Sanitizers / Hardeners
        "-fsanitize=undefined", // UB Sanitizer
        "-fsanitize-trap", // UB Sanitizer handles UB by trapping
        "-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG", // libc++ hardening mode
        "-ftrivial-auto-var-init=pattern", // Overwrites uninitialized memory with a pattern

        // Dialect flags
        "-fno-exceptions", // Disables exception handling
        "-fno-rtti", // Disables runtime type info

        // Useful flags
        "-ftrapv", // Treat signed integer overflow as two’s complement (wraps around)
        "-fstrict-enums", // Enable optimizations based on the strict definition of an enum’s value range
        "-ftime-trace", // Outputs a Chrome Tracing .json object containing a compiler performance report

        // // Flags to DISABLE warnings -- for use with -Werror
        // "-Wno-cast-qual", // Disables warning when a pointer is cast so as to remove a type qualifier
        // "-Wno-conversion", // Disables warning on implicit conversion that may result in data loss
        // "-Wno-float-equal", // Disables warning when floating-point values used in equality comparisons
        // "-Wno-ignored-qualifiers", // Disables warning when return type of a function has a type qualifier
        // "-Wno-non-virtual-dtor", // Disables warning when class has virtual functions but non-virtual destructor
        // "-Wno-sign-compare", // Disables warning when a comparison between signed and unsigned values could produce an incorrect result
        // "-Wno-sign-conversion", // Disables warning on implicit conversion between un/signed
        // "-Wno-unused", // Disables warnings for unused variables, functions etc.
        // "-Wno-unused-parameter", // Disables unused parameter warnings
        // "-Wno-unused-template", // Disables unused template warnings

        // // Converts warnings into errors -- turn off when needed
        // "-Werror",
        // "-ferror-limit=0",

        // // Enable Tracy profiler
        // "-DTRACY_ENABLE",
    };

    // Add include paths (one path per folder containing a #include)
    exe.addIncludePath(b.path("include/"));

    // Link libraries and add .c and .cpp files with compiler flags
    exe.linkLibC(); // when appropriate

    exe.addCSourceFiles(.{
        .files = &exe_files,
        .flags = &exe_flags,
    });

    b.installArtifact(exe);

    ////////////////////////////////////////////////////////////////////////////
    // Define `run` command
    //
    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    // This allows the user to pass arguments to the application in the build
    // command itself, like this: `zig build run -- arg1 arg2 etc`
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
