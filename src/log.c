#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static const char* log_level_str[] = {
    [TRACE] = "TRACE",
    [DEBUG] = "DEBUG",
    [INFO]  = "INFO",
    [WARN]  = "WARN",
    [ERROR] = "ERROR",
    [FATAL] = "FATAL",
};

#define RESET "\x1b[0m"
#define DIM "\x1b[2m"

static const char* log_level_colors[] = {
    [TRACE] = "\x1b[94m",
    [DEBUG] = "\x1b[36m",
    [INFO]  = "\x1b[32m",
    [WARN]  = "\x1b[33m",
    [ERROR] = "\x1b[31m",
    [FATAL] = "\x1b[35m",
};


void log_proc(
    LogLevel level,
    const char* fmt,
    const char* file,
    int line,
    ...
) {
    va_list args;
    va_start(args, line);
    
    fprintf(
        stderr, 
        DIM "[%25s:%-5d" RESET " %s%5s" RESET DIM "] " RESET, 
        file, 
        line,
        log_level_colors[level],
        log_level_str[level]
    );
    vfprintf(stderr, fmt, args);
    fputs("\n", stderr);

    va_end(args);
}
