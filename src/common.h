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

// 获取两个数的最小值
#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))

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

 private:
    static void log_internal(LOG_LEVEL log_level, const char* fmt, va_list ap);
};
