#pragma once

typedef enum {
    LOG_DISABLED = 0,
    LOG_ERROR,
    LOG_INFO,
    LOG_DEBUG,
} LogLevel;

#ifndef LOG_MAX_LEVEL
#define LOG_MAX_LEVEL LOG_DEBUG
#endif

#if LOG_MAX_LEVEL >= LOG_ERROR
#define log_error(...) log_message(LOG_ERROR, __VA_ARGS__)
#else
#define log_error(...)
#endif

#if LOG_MAX_LEVEL >= LOG_INFO
#define log_info(...)  log_message(LOG_INFO, __VA_ARGS__)
#else
#define log_info(...)
#endif

#if LOG_MAX_LEVEL >= LOG_DEBUG
#define log_debug(...) log_message(LOG_DEBUG, __VA_ARGS__)
#else
#define log_debug(...)
#endif

void log_message(LogLevel level, const char* fmt, ...);
