/**
 * @file process_list.h
 * @author zhangyi
 * @brief 所有进程的相关监控信息
 * @version 0.1
 * @date 2022-11-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include <vector>
#include <map>
#include <memory>
#include "process.h"

/**
 * @brief 所有进程的信息收集
 * 
 */
class ProcessListCollection {
 public:
    /**
     * @brief 获取所有进程的监控信息
     * 
     */
    void get_process_list_monitor_info();


 private:
    /**
     * @brief 获取系统整体的内存系统
     * 
     */
    void get_sys_mem_info();

    void get_scan_cpu_time();


 private:
    std::shared_ptr<ProcessList> process_list_;
    std::vector<ProcessInfo> process_vec_;
    std::map<pid_t, ProcessInfo> process_map_;
};

