#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

void Util::fatal_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_internal(LOG_FATAL_LEVEL, fmt, ap);
    va_end(ap);
    exit(-1);
}

void Util::error_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_internal(LOG_ERROR_LEVEL, fmt, ap);
    va_end(ap);
}

void Util::warn_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_internal(LOG_WARN_LEVEL, fmt, ap);
    va_end(ap);
}

void Util::info_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_internal(LOG_INFO_LEVEL, fmt, ap);
    va_end(ap);
}

void Util::debug_log(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    log_internal(LOG_DEBUG_LEVEL, fmt, ap);
    va_end(ap);
}

void Util::log_internal(LOG_LEVEL log_level, const char* fmt, va_list ap) {
    if (log_level > global_log_level) {
        return;
    }
    char buf[1024];
    size_t bufLen = sizeof(buf);
    if (fmt != NULL) {
        vsnprintf(buf, bufLen, fmt, ap);
        buf[bufLen-1] = '\0';
    } else {
        buf[0] = '\0';
    }
    fprintf(stdout, "[top-cpp %s] %s \n", get_log_level_str(log_level), buf);
}

ssize_t Util::read_file(int dir_fd, const char* path_name, void* buffer, size_t count) {
    int fd = openat(dir_fd, path_name, O_RDONLY);
    if (fd < 0) {
        return -errno;
    }
    if (!count) {
        close(fd);
        return -EINVAL;
    }
    ssize_t already_read = 0;
    count--;  // 预留一个空字符
    for (;;) {
        ssize_t res = read(fd, buffer, count);
        if (res == -1) {
            // 如果是中断导致，则在此循环
            if (errno == EINTR) {
                continue;
            }
            close(fd);
            return -errno;
        }
        if (res > 0) {
            buffer = (reinterpret_cast<char*>(buffer)) + res;
            count -= static_cast<size_t>(res);
            already_read += res;
        }
        if (count == 0 || res == 0) {
            close(fd);
            *(reinterpret_cast<char*>(buffer)) = '\0';
            return already_read;
        }
    }
}

FILE* Util::fopenat(int perent_fd, const char* path_name, const char* mode) {
    int fd = openat(perent_fd, path_name, O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }
    FILE* stream = fdopen(fd, mode);
    if (!stream) {
        close(fd);
    }
    return stream;
}

int Util::get_real_time(struct timeval* tvp, uint64_t* msec) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        tvp->tv_sec = ts.tv_sec;
        tvp->tv_usec = ts.tv_nsec / 1000;
        *msec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        return 0;
    } else {
        tvp->tv_sec = 0;
        tvp->tv_usec = 0;
        *msec = 0;
        return -1;
    }
}
