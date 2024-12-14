#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void log_format(const char* tag, const char* message, ...) {
    time_t now;
    time(&now);
    struct tm* time_info = localtime(&now);
    char formatted_date[20];
    strftime(formatted_date, sizeof(formatted_date), "%Y-%m-%d %H:%M:%S", time_info);
    printf("%s (streamd) [%s] ", formatted_date, tag);
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}
