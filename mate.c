#define MATE_IMPLEMENTATION
#include "mate.h"

#include <stdio.h>

#define OUTPUT "cbc"

#ifndef COMPILER
#define COMPILER GCC
#endif

#ifdef DEBUG
#define OPTS                                                                   \
    (ExecutableOptions) {                                                      \
        .output = OUTPUT, .warnings = FLAG_WARNINGS_VERBOSE,                   \
        .error = FLAG_ERROR_MAX, .debug = FLAG_DEBUG, .std = FLAG_STD_C11,     \
        .optimization = FLAG_OPTIMIZATION_NONE,                                \
        .sanitizer = FLAG_SANITIZER_ADDRESS,                                   \
    }
#else
#define OPTS                                                                   \
    (ExecutableOptions) {                                                      \
        .output = OUTPUT, .optimization = FLAG_OPTIMIZATION_AGGRESSIVE,        \
        .std = FLAG_STD_C11                                                    \
    }
#endif

#define CheckRunCommand(s)                                                     \
    if (RunCommand(S(s)) != SUCCESS) {                                         \
        fprintf(stderr, "could not execute command `" s "`\n");                \
        abort();                                                               \
    }

int main(void) {
#ifdef DEBUG
    fprintf(stderr, "compiling with debug flags\n");
#else
    fprintf(stderr, "compiling with release flags\n");
#endif

    CreateConfig(
        (MateOptions){.compiler = COMPILER, .buildDirectory = ".build"});
    StartBuild();

    Executable exec = CreateExecutable(OPTS);

    AddFile(exec, "./src/*.c");

    InstallExecutable(exec);

    EndBuild();
}
