#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

const char* get_log_level_str(LOG_LEVEL log_level) {
    switch (log_level) {
    case LOG_FATAL_LEVEL:
        return "FATAL";
    case LOG_ERROR_LEVEL:
        return "ERROR";
    case LOG_WARN_LEVEL:
        return "WARN";
    case LOG_INFO_LEVEL:
        return "INFO";
    case LOG_DEBUG_LEVEL:
        return "DEBUG";
    default:
        return "UNKOWN_LOG_LEVEL";
    }
}

void Util::log_internal(LOG_LEVEL log_level, const char* fmt, va_list ap) {
    char buf[1024];
    size_t bufLen = sizeof(buf);
    if (fmt != NULL) {
        vsnprintf(buf, bufLen, fmt, ap);
        buf[bufLen-1] = '\0';
    } else {
        buf[0] = '\0';
    }
    fprintf(stderr, "[top-cpp %s] %s \n", get_log_level_str(log_level), buf);
}

void Util::fatal_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_internal(LOG_FATAL_LEVEL, fmt, ap);
    va_end(ap);
    exit(-1);
}

