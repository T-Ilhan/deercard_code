#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum log_level {
    LOG_TYPE_DEBUG,
    LOG_TYPE_OK,
    LOG_TYPE_INFO,
    LOG_TYPE_WARN,
    LOG_TYPE_ERR
} log_level_t;

void log_(log_level_t level, const char* context, const char* msg, bool print_newline, ...);

#ifdef ENABLE_DEBUG
    #define LOG_DEBUG(context, msg, ...) log_(LOG_TYPE_DEBUG, context, msg, true, ## __VA_ARGS__)
#else
    #define LOG_DEBUG(context, msg, ...) do {} while(0);    // no op
#endif
#define LOG_OK(context, msg, ...) log_(LOG_TYPE_OK, context, msg, true, ## __VA_ARGS__)
#define LOG_INFO(context, msg, ...) log_(LOG_TYPE_INFO, context, msg, true, ## __VA_ARGS__)
#define LOG_WARN(context, msg, ...) log_(LOG_TYPE_WARN, context, msg, true, ## __VA_ARGS__)
#define LOG_ERR(context, msg, ...) log_(LOG_TYPE_ERR, context, msg, true, ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif