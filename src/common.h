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

#define FATAL_LOG(note) Util::fatal_log(note)

class Util {
 public:
    static void fatal_log(const char* note);
};
