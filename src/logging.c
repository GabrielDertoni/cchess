#include <stdarg.h>
#include <stdio.h>
#include "logging.h"

void log_message(LogLevel level, const char* fmt, ...) {
    static char const* const level_str[] = {
        [LOG_ERROR] = "error", 
        [LOG_INFO]  = "info", 
        [LOG_DEBUG] = "debug", 
    };
    va_list argp;
    va_start(argp, fmt);
    fprintf(stderr, "[%s] ", level_str[level]);
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n");
    va_end(argp);
}
