#ifndef LOG_H
#define LOG_H

void log_format(const char* tag, const char* message, ...);

#define LOG_LEVEL_ERROR 0
#define LOG_LEVEL_WARN 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define log_error(message, ...) log_format("error", message, ##__VA_ARGS__)
#else
#define log_error(message, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define log_warn(message, ...) log_format("warn", message, ##__VA_ARGS__)
#else
#define log_warn(message, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define log_info(message, ...) log_format("info", message, ##__VA_ARGS__)
#else
#define log_info(message, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define log_debug(message, ...) log_format("debug", message, ##__VA_ARGS__)
#else
#define log_debug(message, ...)
#endif

#endif  // LOG_H
