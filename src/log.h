#pragma once

typedef enum {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
} LogLevel;

void log_proc(
    LogLevel level,
    const char* fmt,
    const char* file,
    int line,
    ...
);

#ifndef NO_LOGGING
#define LOG_PROC(LEVEL, FMT, ...) log_proc(LEVEL, FMT, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG_PROC()
#endif

#define LOG_DEBUG(FMT, ...) LOG_PROC(DEBUG, FMT, ##__VA_ARGS__)
#define LOG_TRACE(FMT, ...) LOG_PROC(TRACE, FMT, ##__VA_ARGS__)
#define LOG_INFO(FMT, ...) LOG_PROC(INFO, FMT, ##__VA_ARGS__)
#define LOG_WARN(FMT, ...) LOG_PROC(WARN, FMT, ##__VA_ARGS__)
