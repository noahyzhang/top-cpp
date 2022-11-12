/**
 * @file process.h
 * @author zhangyi
 * @brief 进程相关结构
 * @version 0.1
 * @date 2022-11-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief 定义 /proc/xxx 文件系统的存储目录
 * 
 */
#define PROC_DIR "/proc"
#define PROC_CPUINFO_FILE PROC_DIR "/cpuinfo"
#define PROC_STAT_FILE PROC_DIR "/stat"
#define PROC_MEMINFO_FILE PROC_DIR "/meminfo"

typedef struct ProcessInfo {
    // 进程的标志
    pid_t pid;
    // 当前进程的父进程
    pid_t ppid;
    // 线程组标志
    pid_t tgid;
    // 进程组标志
    pid_t pgrp;
    // session 标志
    int session;
    // 是否为内核线程
    bool is_kernel_thread;
    // 是否为用户空间线程
    bool is_userland_thread;
    // 进程运行时间（以百分之一秒为单位）
    uint64_t time;
    // 进程名（包括参数）
    char* cmdline;
};

