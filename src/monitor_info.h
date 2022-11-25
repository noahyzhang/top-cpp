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
#include <vector>
#include <memory>
#include <unordered_map>

/**
 * @brief 定义 /proc/xxx 文件系统的存储目录
 * 
 */
#define PROC_DIR "/proc"
#define PROC_CPUINFO_FILE PROC_DIR "/cpuinfo"
#define PROC_STAT_FILE PROC_DIR "/stat"
#define PROC_MEMINFO_FILE PROC_DIR "/meminfo"

// proc 文件系统每行最大的长度
#define PROC_LINE_MAX_LENGTH 4096
// 单次读文件的最大字节数
#define MAX_BYTES_ONCE_READ 2048
// 任务名字最大的长度
#define MAX_COMMAND_LENGTH 128

/**
 * @brief 一个 cpu 的所有的监控信息
 * 
 */
struct CpuData {
    uint64_t total_time;
    uint64_t user_time;
    uint64_t system_time;
    uint64_t system_all_time;
    uint64_t idle_all_time;
    uint64_t idle_time;
    uint64_t nice_time;
    uint64_t io_wait_time;
    uint64_t irq_time;
    uint64_t soft_irq_time;
    uint64_t steal_time;
    uint64_t guest_time;

    uint64_t total_period;
    uint64_t user_period;
    uint64_t system_period;
    uint64_t system_all_period;
    uint64_t idle_all_period;
    uint64_t idle_period;
    uint64_t nice_period;
    uint64_t io_wait_period;
    uint64_t irq_period;
    uint64_t soft_irq_period;
    uint64_t steal_period;
    uint64_t guest_period;

    // double frequency;
    bool on_line = false;
};

/**
 * @brief 一个进程的所有的监控信息
 * 
 */
struct ProcessInfo {
    // 进程的标志
    pid_t pid;
    // 当前进程的父进程
    pid_t ppid;
    // // 线程组标志
    // pid_t tgid;
    // // 进程组标志
    // pid_t pgrp;
    // // session 标志
    // int session;
    // // 是否为内核线程
    // bool is_kernel_thread;
    // // 是否为用户空间线程
    // bool is_userland_thread;

    // 进程运行时间（以百分之一秒为单位）
    // uint64_t time;
    // 进程名（包括参数）
    char cmdline[MAX_COMMAND_LENGTH+1];

    /* ---------- 任务的 cpu 相关统计 -------------- */
    // 任务运行在用户态的时间（包括 guest_time，被虚拟机抢占的时间）
    uint64_t utime;
    // 任务运行在内核态的时间
    uint64_t stime;
    // 任务等待子任务被调度在用户态的时间（包括 cguest_time，被虚拟机抢占的时间）
    uint64_t cutime;
    // 任务等待子任务被调度在内核态的时间
    uint64_t cstime;
    // 上一个周期的 CPU 使用率(百分比)
    float percent_cpu;

    /* ---------- 任务的内存相关统计 -------------- */
    // 虚拟内存大小（单位为 KB）
    uint64_t virtual_mem;
    // 常驻内存大小（单位为 KB）
    uint64_t resident_mem;
    // 共享内存大小（单位为 KB）
    uint64_t shared_mem;
    // text (code)
    uint64_t text_mem;
    // data + stack
    uint64_t data_mem;
    // 上一个周期的内存使用率（百分比）
    float percent_mem;

    /* ---------- 任务的 IO 相关统计 -------------- */
    // 读 IO 的字节数，包含 pagecache
    uint64_t io_read_char;
    // 写 IO 的字节数，包含 pagecache
    uint64_t io_write_char;
    // 读 IO 操作的次数，例如：read、pread
    uint64_t io_read_syscalls;
    // 写 IO 操作的次数，例如：write、pwrite
    uint64_t io_write_syscalls;
    // 实际读磁盘的字节数
    uint64_t io_read_bytes;
    // 实际写磁盘的字节数
    uint64_t io_write_bytes;
    // 取消写的字节数
    uint64_t io_cancelled_write_bytes;
    // 最后一次 IO 扫描的时间点
    uint64_t io_last_scan_time_ms;
    // 实际读磁盘的速率（字节每秒）
    double io_rate_read_bps;
    // 实际写磁盘的速率（字节每秒）
    double io_rate_write_bps;
};

/**
 * @brief 当前系统的所有监控信息
 * 
 */
struct SysMonitorInfo {
    // 当前时间
    struct timeval curr_real_time;
    // 当前时间，单位为 ms
    uint64_t curr_time_ms;

    // 当前系统的可用内存，单位为 kb
    uint64_t available_mem;

    uint64_t total_mem;
    uint64_t used_mem;
    uint64_t buffers_mem;
    uint64_t cached_mem;
    uint64_t shared_mem;
    uint64_t avilable_mem;
    uint64_t total_swap;
    uint64_t used_swap;
    uint64_t cached_swap;

    // 活跃的 cpu 个数
    uint32_t active_cpus;
    // 存在的 cpu 个数
    uint32_t existing_cpus;
    // 当前系统上每个 cpu 的数据
    std::vector<std::shared_ptr<CpuData>> sys_cpu_data;

    // 当前系统上所有进程的监控数据
    std::vector<std::shared_ptr<ProcessInfo>> all_process_info;
    // 所有进程的监控数据（以 pid 做为 key）
    std::unordered_map<uint64_t, std::shared_ptr<ProcessInfo>> all_process_info_table;

    SysMonitorInfo()
        : curr_time_ms(0),
          available_mem(0),
          total_mem(0),
          used_mem(0),
          buffers_mem(0),
          cached_mem(0),
          shared_mem(0),
          avilable_mem(0),
          total_swap(0),
          used_swap(0),
          cached_swap(0),
          active_cpus(0),
          existing_cpus(0) {}
};
