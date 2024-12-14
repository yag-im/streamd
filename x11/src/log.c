#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

#if defined(LOG_LEVEL)
#if LOG_LEVEL == LOG_LEVEL_ERROR
#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR
#elif LOG_LEVEL == LOG_LEVEL_WARN
#define CURRENT_LOG_LEVEL LOG_LEVEL_WARN
#elif LOG_LEVEL == LOG_LEVEL_INFO
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#elif LOG_LEVEL == LOG_LEVEL_DEBUG
#define CURRENT_LOG_LEVEL LOG_LEVEL_DEBUG
#else
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif
#else
#define CURRENT_LOG_LEVEL LOG_LEVEL_INFO
#endif

void log_format(const char* tag, const char* message, va_list args) {
    int log_level = -1;
    if (strcmp(tag, "info") == 0) {
        log_level = LOG_LEVEL_INFO;
    } else if (strcmp(tag, "debug") == 0) {
        log_level = LOG_LEVEL_DEBUG;
    } else if (strcmp(tag, "warn") == 0) {
        log_level = LOG_LEVEL_WARN;
    } else if (strcmp(tag, "error") == 0) {
        log_level = LOG_LEVEL_ERROR;
    }
    if (log_level <= CURRENT_LOG_LEVEL) {
        time_t now;
        time(&now);
        char* date = ctime(&now);
        date[strlen(date) - 1] = '\0';
        printf("streamd: %s [%s] ", date, tag);
        vprintf(message, args);
        printf("\n");
    }
}

void log_error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_format("error", message, args);
    va_end(args);
}

void log_warn(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_format("warn", message, args);
    va_end(args);
}

void log_info(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_format("info", message, args);
    va_end(args);
}

void log_debug(const char* message, ...) {
    va_list args;
    va_start(args, message);
    log_format("debug", message, args);
    va_end(args);
}
