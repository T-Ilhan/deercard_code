#include "logging.h"

#define CLEAR   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define CYAN    "\x1b[36m"
#define GRAY    "\x1b[90m"

#define LEVEL_DEBUG "[DBG ] "
#define LEVEL_NONE  "[ -  ] "
#define LEVEL_INFO  "[INFO] "
#define LEVEL_WARN  "[WARN] "
#define LEVEL_ERR   "[ERR ] "

int _write(int file, char *data, int len){
    for (int i = 0; i < len; i++) {
        Serial.write(data[i]);
    }
    return len;
}

void log_(log_level_t level, const char* context, const char* msg, bool print_newline, ...) {
    switch (level) {
        case LOG_TYPE_DEBUG:
            printf(GRAY LEVEL_DEBUG "%s: ", context);
            break;
        case LOG_TYPE_OK:
            printf(GREEN LEVEL_NONE "%s: " CLEAR, context);
            break;
        case LOG_TYPE_INFO:
            printf(CYAN LEVEL_INFO "%s: " CLEAR, context);
            break;
        case LOG_TYPE_WARN:
            printf(YELLOW LEVEL_WARN "%s: " CLEAR, context);
            break;
        case LOG_TYPE_ERR:
            printf(RED LEVEL_ERR "%s: " CLEAR, context);
            break;
    }

    va_list args;
    va_start(args, print_newline);

    vprintf(msg, args);
    printf(print_newline ? CLEAR "\n" : CLEAR);

    va_end(args);
}