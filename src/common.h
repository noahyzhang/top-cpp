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

// 获取两个数的最小值
#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))

// 封装日志宏
#define FATAL_LOG(note) Util::fatal_log(note)
#define ERROR_LOG(note) Util::error_log(note)
#define WARN_LOG(note) Util::warn_log(note)
#define INFO_LOG(note) Util::info_log(note)
#define DEBUG_LOG(note) Util::debug_log(note)

class Util {
 public:
    static void fatal_log(const char* note);
    static void error_log(const char* note);
    static void warn_log(const char* note);
    static void info_log(const char* note);
    static void debug_log(const char* note);
};
