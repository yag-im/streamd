#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void log_format(const char* tag, const char* message, ...) {
    time_t now;
    time(&now);
    char* date = ctime(&now);
    date[strlen(date) - 1] = '\0';
    printf("streamd: %s [%s] ", date, tag);

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    printf("\n");
}
