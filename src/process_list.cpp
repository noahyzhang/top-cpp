#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include "common.h"
#include "process_list.h"

void ProcessListCollection::get_process_list_monitor_info() {

}

void ProcessListCollection::get_sys_mem_info() {
    uint64_t available_mem = 0;  // 可用的内存
    uint64_t free_mem = 0;  // 空闲的内存
    uint64_t total_mem = 0;  // 总内存
    uint64_t buffers_mem = 0;  // buffers 缓冲内存
    uint64_t cached_mem = 0;  // cached 缓冲内存
    uint64_t shared_mem = 0;  // 共享内存
    uint64_t swap_total_mem = 0;  // swap 总内存
    uint64_t swap_cache_mem = 0;  // swap 缓冲内存
    uint64_t swap_free_mem = 0;  // swap 空闲内存
    uint64_t sreclaimable_mem = 0;  // 可回收的内存

    FILE* file = fopen(PROC_MEMINFO_FILE, "r");
    if (!file) {
        FATAL_LOG("cannot open file:" PROC_MEMINFO_FILE);
    }
    // 读取 /proc/meminfo 中的数据
    char buf[128];
    while (fgets(buf, sizeof(buf), file)) {
        // 使用宏定义，定义读取文件的每一行
        #define try_read(label, variable)                                     \
            if (strncmp(buf, label, strlen(label)) == 0) {                    \
                uint64_t parse_var;                                           \
                if (sscanf(buf+strlen(label), "%llu KB", &parse_var) == 1) {  \
                    (variable) = parse_var;                                   \
                }                                                             \
                break;                                                        \
            } else (void) 0

        switch (buf[0]) {
        case 'M':
            try_read("MemAvailable", available_mem);
            try_read("MemFree", free_mem);
            try_read("MemTotal", total_mem);
            break;
        case 'B':
            try_read("Buffers", buffers_mem);
            break;
        case 'C':
            try_read("Cached", cached_mem);
            break;
        case 'S':
            switch (buf[1]) {
            case 'h':
                try_read("Shmem", shared_mem);
                break;
            case 'w':
                try_read("SwapTotal", swap_total_mem);
                try_read("SwapCached", swap_cache_mem);
                try_read("SwapFree", swap_free_mem);
                break;
            case 'R':
                try_read("SReclaimable", sreclaimable_mem);
                break;
            }
            break;
        }
        #undef try_read
    }
    fclose(file);
    process_list_->total_mem = total_mem;
    process_list_->cached_mem = cached_mem + sreclaimable_mem - shared_mem;
    process_list_->shared_mem = shared_mem;
    const uint64_t used_diff = free_mem + cached_mem + sreclaimable_mem + buffers_mem;
    process_list_->used_mem = (total_mem >= used_diff) ? (total_mem - used_diff) : (total_mem - free_mem);
    process_list_->buffers_mem = buffers_mem;
    process_list_->avilable_mem = available_mem != 0 ? MINIMUM(available_mem, total_mem) : free_mem;
    process_list_->total_swap = swap_total_mem;
    process_list_->used_swap = swap_total_mem - swap_free_mem - swap_cache_mem;
    process_list_->cached_swap = swap_cache_mem;
    return;
}

void ProcessListCollection::get_scan_cpu_time() {
    uint64_t existing = 0, active = 0;
    
    DIR* dir = opendir("/sys/devices/system/cpu");
    if (!dir) {
        FATAL_LOG("")
    }
}
