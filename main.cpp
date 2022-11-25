#include <unistd.h>
#include <iostream>
#include "monitor_info_collect.h"
#include "common.h"

int main() {
    int res = MonitorInfoCollection::get_instance().initialize();
    if (res < 0) {
        FATAL_LOG("MonitorInfoCollection init failed");
    }
    for (;;) {
        auto monitor_info = MonitorInfoCollection::get_instance().finish_once_monitor();
        if (monitor_info == nullptr) {
            std::cout << "finish once monitor failed" << std::endl;
            return -1;
        }
        // std::cout << monitor_info->existing_cpus << std::endl;
        // std::cout << monitor_info->sys_cpu_data[1]->total_period << std::endl;

        // 输出 cpu 汇总值
        double total_cpu = 1;
        if (monitor_info->sys_cpu_data[0]->total_period != 0) {
            total_cpu = monitor_info->sys_cpu_data[0]->total_period;
        }
        double cpu_percent = (total_cpu - monitor_info->sys_cpu_data[0]->idle_all_period) / total_cpu;
        std::cout << "cpu total usage: " << cpu_percent << std::endl;

        // 输出每个 pid 的监控
        for (const auto& task_info : monitor_info->all_process_info) {
            if (task_info->percent_cpu > 0.0001) {
                std::cout << task_info->pid << ", cmdline: " << task_info->cmdline
                    << ", cpu usage: " << task_info->percent_cpu
                    << ", mem usage: " << task_info->percent_mem << std::endl;
            }
        }
        sleep(2);
    }
}
