/**
 * @file process_list.h
 * @author zhangyi
 * @brief 监控信息收集
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
#include "monitor_info.h"

/**
 * @brief 监控信息收集
 * 
 */
class MonitorInfoCollection {
 public:
    /**
     * @brief 单例模式
     * 
     * @return 全局唯一对象
     */
    static MonitorInfoCollection& get_instance() {
       static MonitorInfoCollection instance;
       return instance;
    }
    ~MonitorInfoCollection() = default;
    MonitorInfoCollection(const MonitorInfoCollection&) = delete;
    MonitorInfoCollection& operator=(const MonitorInfoCollection&) = delete;
    MonitorInfoCollection(const MonitorInfoCollection&&) = delete;
    MonitorInfoCollection& operator=(MonitorInfoCollection&&) = delete;

    /**
     * @brief 初始化函数
     * @note 对象申请之后必须要调用初始化函数
     * @return int 初始化是否成功
     */
    int initialize();

 public:
    /**
     * @brief 完成一次监控
     * 
     * @return int 
     */
    const std::shared_ptr<SysMonitorInfo> finish_once_monitor();

 private:
    /**
     * @brief 获取系统整体的内存信息
     * @note 通过读取 /proc/meminfo 中的数据
     */
    int get_sys_mem_info();

    /**
     * @brief 获取系统整体的 cpu 信息
     * @note 每个 cpu 的信息
     */
    int get_sys_cpu_info();

    /**
     * @brief 递归的获取所有的进程占用资源信息
     * @note 辅助函数
     */
    int get_all_process_info_recurse(int parent_fd, const char* dir_name,
        const std::shared_ptr<ProcessInfo>& parent);

    void get_task_io_info(int proc_fd, std::shared_ptr<ProcessInfo> process);

    int get_task_statm_info(int proc_fd, std::shared_ptr<ProcessInfo> process);

    int get_task_stat_info(int proc_fd, std::shared_ptr<ProcessInfo> process);

 private:
    /**
     * @brief 更新 cpu 的数量
     * 
     */
    void update_cpu_count();

    std::shared_ptr<ProcessInfo> get_process_info(uint64_t pid, bool* prev_existed);


    inline uint64_t adjust_time(uint64_t tm) {
       return tm * 100 / jiffy_;
    }

 private:
    MonitorInfoCollection() {
       sys_monitor_info_ = std::make_shared<SysMonitorInfo>();
    }


 private:
    std::shared_ptr<SysMonitorInfo> sys_monitor_info_;

    double period_;
    int page_size_kb_;  // 一个 page 的大小
    uint64_t jiffy_;  // 一个时间周期的时长
};

