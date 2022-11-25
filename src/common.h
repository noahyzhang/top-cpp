/**
 * @file common.h
 * @author zhangyi
 * @brief 封装一些公共的工具函数
 * @version 0.1
 * @date 2022-11-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <stdarg.h>
#include <string.h>
#include <memory>

// 获取两个数的最小值
#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))
// 获取两个数的最大值
#define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))

// 定义日志级别
enum LOG_LEVEL {
    LOG_FATAL_LEVEL = 0,
    LOG_ERROR_LEVEL,
    LOG_WARN_LEVEL,
    LOG_INFO_LEVEL,
    LOG_DEBUG_LEVEL,
};

// 封装日志宏
#define FATAL_LOG(fmt, ...) Util::fatal_log(fmt, ##__VA_ARGS__)
#define ERROR_LOG(fmt, ...) Util::error_log(fmt, ##__VA_ARGS__)
#define WARN_LOG(fmt, ...) Util::warn_log(fmt, ##__VA_ARGS__)
#define INFO_LOG(fmt, ...) Util::info_log(fmt, ##__VA_ARGS__)
#define DEBUG_LOG(fmt, ...) Util::debug_log(fmt, ##__VA_ARGS__)

static LOG_LEVEL global_log_level = LOG_INFO_LEVEL;

/**
 * @brief 工具类，提供需要的工具函数
 * 
 */
class Util {
 public:
    static void fatal_log(const char* fmt, ...);
    static void error_log(const char* fmt, ...);
    static void warn_log(const char* fmt, ...);
    static void info_log(const char* fmt, ...);
    static void debug_log(const char* fmt, ...);

    static ssize_t read_file(int dir_fd, const char* path_name, void* buffer, size_t count);
    static inline bool wrap_strncmp(const char* s, const char* match) {
        return strncmp(s, match, strlen(match)) == 0;
    }
    static FILE* fopenat(int fd, const char* path_name, const char* mode);

    static int get_real_time(struct timeval* tvp, uint64_t* msec);

 private:
    static void log_internal(LOG_LEVEL log_level, const char* fmt, va_list ap);
};
