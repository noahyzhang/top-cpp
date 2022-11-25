#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include "common.h"
#include "monitor_info_collect.h"

int MonitorInfoCollection::initialize() {
    // 初始化 page_size_kb_
    int page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        ERROR_LOG("get pagesize by sysconf failed, err: %s", strerror(errno));
        return -1;
    }
    page_size_kb_ = page_size / 1024UL;
    // 初始化 jiffy_
    jiffy_ = sysconf(_SC_CLK_TCK);
    if (jiffy_ < 0) {
        ERROR_LOG("get clk_tck by sysconf failed, err: %s", strerror(errno));
        return -2;
    }
    // 获取 cpu 的数量
    update_cpu_count();
    return 0;
}

const std::shared_ptr<SysMonitorInfo> MonitorInfoCollection::finish_once_monitor() {
    // 初始化时间
    if (Util::get_real_time(&sys_monitor_info_->curr_real_time,
        &sys_monitor_info_->curr_time_ms) < 0) {
        ERROR_LOG("get time failed, err: %s", strerror(errno));
        return std::shared_ptr<SysMonitorInfo>();
    }
    // 获取系统整体的内存监控信息
    if (get_sys_mem_info() < 0) return std::shared_ptr<SysMonitorInfo>();
    // 获取系统每个 cpu 的监控信息
    if (get_sys_cpu_info() < 0) return std::shared_ptr<SysMonitorInfo>();
    // 递归的获取每个进程的监控信息
    get_all_process_info_recurse(AT_FDCWD, PROC_DIR, nullptr);
    return sys_monitor_info_;
}

int MonitorInfoCollection::get_sys_mem_info() {
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
        ERROR_LOG("cannot open file: %s, err: %s", PROC_MEMINFO_FILE, strerror(errno));
        return -1;
    }
    // 读取 /proc/meminfo 中的数据
    char buf[128];
    while (fgets(buf, sizeof(buf), file)) {
        // 使用宏定义，定义读取文件的每一行
        #define try_read(label, variable)                                     \
            if (strncmp(buf, label, strlen(label)) == 0) {                    \
                uint64_t parse_var;                                           \
                if (sscanf(buf+strlen(label), "%lu KB", &parse_var) == 1) {  \
                    (variable) = parse_var;                                   \
                }                                                             \
                break;                                                        \
            }

        switch (buf[0]) {
        case 'M':
            try_read("MemAvailable:", available_mem);
            try_read("MemFree:", free_mem);
            try_read("MemTotal:", total_mem);
            break;
        case 'B':
            try_read("Buffers:", buffers_mem);
            break;
        case 'C':
            try_read("Cached:", cached_mem);
            break;
        case 'S':
            switch (buf[1]) {
            case 'h':
                try_read("Shmem:", shared_mem);
                break;
            case 'w':
                try_read("SwapTotal:", swap_total_mem);
                try_read("SwapCached:", swap_cache_mem);
                try_read("SwapFree:", swap_free_mem);
                break;
            case 'R':
                try_read("SReclaimable:", sreclaimable_mem);
                break;
            }
            break;
        }
        #undef try_read
    }
    fclose(file);
    sys_monitor_info_->total_mem = total_mem;
    sys_monitor_info_->cached_mem = cached_mem + sreclaimable_mem - shared_mem;
    sys_monitor_info_->shared_mem = shared_mem;
    const uint64_t used_diff = free_mem + cached_mem + sreclaimable_mem + buffers_mem;
    sys_monitor_info_->used_mem = (total_mem >= used_diff) ? (total_mem - used_diff) : (total_mem - free_mem);
    sys_monitor_info_->buffers_mem = buffers_mem;
    sys_monitor_info_->avilable_mem = available_mem != 0 ? MINIMUM(available_mem, total_mem) : free_mem;
    sys_monitor_info_->total_swap = swap_total_mem;
    sys_monitor_info_->used_swap = swap_total_mem - swap_free_mem - swap_cache_mem;
    sys_monitor_info_->cached_swap = swap_cache_mem;
    return 0;
}

void MonitorInfoCollection::update_cpu_count() {
    uint32_t existing_cpus = 0, active_cpus = 0;
    DIR* dir = opendir("/sys/devices/system/cpu");
    if (!dir) {
        ERROR_LOG("open dir: %s failed, err: %s", "/sys/devices/system/cpu", strerror(errno));
        return;
    }
    if (sys_monitor_info_->sys_cpu_data.empty()) {
        auto cpu_data = std::make_shared<CpuData>();
        cpu_data->on_line = true;
        sys_monitor_info_->sys_cpu_data.emplace_back(cpu_data);
    }
    uint32_t curr_existing_cpu = sys_monitor_info_->existing_cpus;
    const struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // 如果不是目录或者类型未知，则跳过
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) continue;
        // 如果目录名不是 cpu* ，则跳过
        uint32_t cpu_str_len = strlen("cpu");
        if (strncmp(entry->d_name, "cpu", cpu_str_len) != 0) continue;
        // 获取 cpu 的 id，比如 cpu1，id 则为 1
        char* endp;
        uint64_t id = strtoul(entry->d_name + cpu_str_len, &endp, 10);
        // 如果 id 值太大，或者目录名 cpu* 没有数字，或者不合法。则跳过
        if (id == UINT64_MAX || endp == entry->d_name+cpu_str_len || *endp != '\0') continue;
        // 获取 "/sys/devices/system/cpu/" 目录下的子目录
        int cpu_dir_fd = openat(dirfd(dir), entry->d_name, O_DIRECTORY | O_PATH | O_NOFOLLOW);
        if (cpu_dir_fd < 0) continue;
        // 这里找到了某个 cpu* ,则存在的 cpu 数目加一
        existing_cpus++;
        // 更新数组的长度
        uint32_t max_val = MAXIMUM(existing_cpus, id+1);
        if (max_val > curr_existing_cpu) {
            for (size_t i = curr_existing_cpu+1; i <= max_val; i++) {
                auto cpu_data = std::make_shared<CpuData>();
                sys_monitor_info_->sys_cpu_data.emplace_back(cpu_data);
            }
            curr_existing_cpu = max_val;
        }
        // 判断此 cpu 是否在线
        char buffer[8];
        ssize_t res = Util::read_file(cpu_dir_fd, "online", buffer, sizeof(buffer));
        if (res < 1 || buffer[0] != '0') {
            active_cpus++;
            sys_monitor_info_->sys_cpu_data[id+1]->on_line = true;
        } else {
            sys_monitor_info_->sys_cpu_data[id+1]->on_line = false;
        }
        close(cpu_dir_fd);
    }
    closedir(dir);
    if (existing_cpus < 1) return;
    // 需要多分配一个元素，存储总 cpu 的数据
    sys_monitor_info_->sys_cpu_data.resize(existing_cpus+1);
    sys_monitor_info_->existing_cpus = existing_cpus;
    sys_monitor_info_->active_cpus = active_cpus;
    return;
}

int MonitorInfoCollection::get_sys_cpu_info() {
    // 通过 /proc/stat 文件获取系统 cpu 信息
    FILE* file = fopen(PROC_STAT_FILE, "r");
    if (!file) {
        ERROR_LOG("open file: %s failed, err: %s", PROC_STAT_FILE, strerror(errno));
        return -1;
    }
    // 获取每个 cpu 的信息
    for (size_t i = 0; i <= sys_monitor_info_->existing_cpus; i++) {
        char buffer[PROC_LINE_MAX_LENGTH + 1];
        uint64_t user_time = 0, nice_time = 0, system_time = 0, idle_time = 0;
        uint64_t io_wait = 0, irq = 0, soft_irq = 0, steal = 0, guest = 0, guest_nice = 0;
        const char* is_succ = fgets(buffer, sizeof(buffer), file);
        if (!is_succ) {
            ERROR_LOG("fgets file failed, err: %s", strerror(errno));
            break;
        }
        if (strncmp(buffer, "cpu", strlen("cpu")) != 0) break;

        uint32_t adj_cpu_id = 0;
        int res = 0;
        if (i == 0) {
            res = sscanf(buffer, "cpu  %16lu %16lu %16lu %16lu %16lu %16lu %16lu %16lu %16lu %16lu",
                &user_time, &nice_time, &system_time, &idle_time,
                &io_wait, &irq, &soft_irq, &steal, &guest, &guest_nice);
            if (res <= 0) {
                ERROR_LOG("sscanf cpu failed, err: %s", strerror(errno));
                continue;
            }
            adj_cpu_id = 0;
        } else {
            uint32_t cpu_id;
            res = sscanf(buffer, "cpu%4u %16lu %16lu %16lu %16lu %16lu %16lu %16lu %16lu %16lu %16lu",
                &cpu_id, &user_time, &nice_time, &system_time, &idle_time,
                &io_wait, &irq, &soft_irq, &steal, &guest, &guest_nice);
            if (res <= 0) {
                ERROR_LOG("sscanf cpu%d failed, err: %s", cpu_id, strerror(errno));
                continue;
            }
            adj_cpu_id = cpu_id + 1;
        }
        if (adj_cpu_id > sys_monitor_info_->existing_cpus || adj_cpu_id >= sys_monitor_info_->sys_cpu_data.size()) {
            break;
        }
        // guest 时间已经被计算在 user_time 中了
        user_time -= guest;
        nice_time -= guest_nice;
        uint64_t idle_all_time = idle_time + io_wait;
        uint64_t system_all_time = system_time + irq + soft_irq;
        uint64_t virtual_all_time = guest + guest_nice;
        uint64_t total_time = user_time + nice_time + system_all_time + idle_all_time + steal + virtual_all_time;

        auto tmp_cpu_data = sys_monitor_info_->sys_cpu_data[adj_cpu_id];
        #define ULL_VALUE_SUB(a, b) (((a) > (b)) ? ((a)-(b)) : 0)
        // 计算一个周期的 cpu 相关属性值
        tmp_cpu_data->user_period = ULL_VALUE_SUB(user_time, tmp_cpu_data->user_time);
        tmp_cpu_data->nice_period = ULL_VALUE_SUB(nice_time, tmp_cpu_data->nice_time);
        tmp_cpu_data->system_period = ULL_VALUE_SUB(system_time, tmp_cpu_data->system_time);
        tmp_cpu_data->system_all_period = ULL_VALUE_SUB(system_all_time, tmp_cpu_data->system_all_time);
        tmp_cpu_data->idle_all_period = ULL_VALUE_SUB(idle_all_time, tmp_cpu_data->idle_all_time);
        tmp_cpu_data->idle_period = ULL_VALUE_SUB(idle_time, tmp_cpu_data->idle_time);
        tmp_cpu_data->io_wait_period = ULL_VALUE_SUB(io_wait, tmp_cpu_data->io_wait_time);
        tmp_cpu_data->irq_period = ULL_VALUE_SUB(irq, tmp_cpu_data->irq_time);
        tmp_cpu_data->soft_irq_period = ULL_VALUE_SUB(soft_irq, tmp_cpu_data->soft_irq_time);
        tmp_cpu_data->steal_period = ULL_VALUE_SUB(steal, tmp_cpu_data->steal_time);
        tmp_cpu_data->guest_period = ULL_VALUE_SUB(virtual_all_time, tmp_cpu_data->guest_time);
        tmp_cpu_data->total_period = ULL_VALUE_SUB(total_time, tmp_cpu_data->total_time);
        #undef ULL_VALUE_SUB
        DEBUG_LOG("cpu id: %d, total: %lu, user: %lu, nice: %lu, system: %lu, idle: %lu", adj_cpu_id,
            tmp_cpu_data->total_period, tmp_cpu_data->user_period, tmp_cpu_data->nice_period,
            tmp_cpu_data->system_period, tmp_cpu_data->idle_period);
        // 保存当前时刻的值
        tmp_cpu_data->user_time = user_time;
        tmp_cpu_data->nice_time = nice_time;
        tmp_cpu_data->system_time = system_time;
        tmp_cpu_data->system_all_time = system_all_time;
        tmp_cpu_data->idle_all_time = idle_all_time;
        tmp_cpu_data->idle_time = idle_time;
        tmp_cpu_data->io_wait_time = io_wait;
        tmp_cpu_data->irq_time = irq;
        tmp_cpu_data->soft_irq_time = soft_irq;
        tmp_cpu_data->steal_time = steal;
        tmp_cpu_data->guest_time = virtual_all_time;
        tmp_cpu_data->total_time = total_time;
    }
    fclose(file);
    period_ = static_cast<double>(sys_monitor_info_->sys_cpu_data[0]->total_period) / sys_monitor_info_->active_cpus;
    return 0;
}

int MonitorInfoCollection::get_all_process_info_recurse(int parent_fd,
    const char* dir_name, const std::shared_ptr<ProcessInfo>& parent) {
    int dir_fd = openat(parent_fd, dir_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (dir_fd < 0) {
        ERROR_LOG("openat parent_fd: %d, dir_name: %s failed, err: %s", parent_fd, dir_name, strerror(errno));
        return -1;
    }
    DIR* dir = fdopendir(dir_fd);
    if (!dir) {
        close(dir_fd);
        ERROR_LOG("fdopendir: %s failed, err: %s", dir_fd, strerror(errno));
        return -2;
    }
    // 读取 proc 文件系统中进程或线程的目录中的文件信息
    const struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过非目录
        if (entry->d_type != DT_DIR && entry->d_type != DT_UNKNOWN) {
            continue;
        }
        const char* name = entry->d_name;
        // RedHat 会使用点来隐藏线程，这里进行兼容
        if (name[0] == '.') {
            name++;
        }
        // 跳过名字为非数字的目录，任务的目录一定以数字（pid）命名
        if (name[0] < '0' || name[0] > '9') {
            continue;
        }
        // 文件的名字应该为一个数字，表明是进程的目录
        char* end_ptr;
        uint64_t pid = strtoul(name, &end_ptr, 10);
        if (pid == 0 || pid == UINT64_MAX || *end_ptr != '\0') {
            continue;
        }
        // 这里是线程，并且跳过主线程，与进程是同一个 id
        if (parent && pid == parent->pid) {
            continue;
        }
        // 获取进程 "/proc/pid" 对应的目录的 fd
        int proc_fd = openat(dir_fd, entry->d_name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
        if (proc_fd < 0) {
            ERROR_LOG("openat dir_fd: %d, file: %s failed, err: %s", dir_fd, entry->d_name, strerror(errno));
            continue;
        }
        // 获取 ProcessInfo 对象，如果不存在则创建
        bool pid_prev_existed = false;
        auto proc = get_process_info(pid, &pid_prev_existed);
        // 递归获取进程中的所有的线程的监控信息
        // 如果任务是线程，则不需要递归了
        if (!Util::wrap_strncmp(dir_name, "task")) {
            get_all_process_info_recurse(proc_fd, "task", proc);
        }
        // 获取任务的 IO 监控信息
        get_task_io_info(proc_fd, proc);
        /**
         * 也可以读取 smaps 或 smaps_rollup 文件来获取内存相关信息，
         * 但是读取 smaps 或 smaps_rollup 文件很慢、很耗费性能。因此放弃
         * 原因大概为：读取 smaps 的成本与进程的内存使用相关。看起来内核需要检查每个内存页的状态才能生成内容
         * 参见 man 手册 /proc/[pid]/smaps
         */
        // 获取任务中的 statm 监控信息（主要是内存信息）
        if (get_task_statm_info(proc_fd, proc) < 0)
            goto errorReadingProcess;
        {
            // 在读取 stat 文件前先保存上一个周期的任务 cpu 耗时
            uint64_t last_time = proc->utime + proc->stime;
            // 获取任务中的 stat 文件监控信息
            if (get_task_stat_info(proc_fd, proc) < 0)
                goto errorReadingProcess;
            // 计算 cpu、mem 的周期百分比
            float percent_cpu = (period_ < 1E-6) ? 0.0F : ((proc->utime + proc->stime - last_time) / period_ * 100.0);
            proc->percent_cpu = (percent_cpu > sys_monitor_info_->active_cpus * 100.0F) ? (
                sys_monitor_info_->active_cpus * 100.0F) : (MAXIMUM(percent_cpu, 0.0F));
            proc->percent_mem = proc->resident_mem / static_cast<double>(sys_monitor_info_->total_mem) * 100.0;
        }
        // 添加 ProcessInfo
        // 如果这个任务是一个新任务
        if (!pid_prev_existed) {
            sys_monitor_info_->all_process_info.emplace_back(proc);
            sys_monitor_info_->all_process_info_table.emplace(pid, proc);
        }
        close(proc_fd);
        continue;

errorReadingProcess:
        {
            if (proc_fd >= 0) {
                close(proc_fd);
            }
        }
    }
    closedir(dir);
    return 0;
}

void MonitorInfoCollection::get_task_io_info(int proc_fd, std::shared_ptr<ProcessInfo> process) {
    char buffer[1024];
    ssize_t res = Util::read_file(proc_fd, "io", buffer, sizeof(buffer));
    if (res < 0) {
        process->io_rate_read_bps = NAN;
        process->io_rate_write_bps = NAN;
        process->io_read_char = 0;
        process->io_write_char = 0;
        process->io_read_syscalls = 0;
        process->io_write_syscalls = 0;
        process->io_read_bytes = 0;
        process->io_write_bytes = 0;
        process->io_cancelled_write_bytes = 0;
        process->io_last_scan_time_ms = sys_monitor_info_->curr_time_ms;
        return;
    }

    uint64_t last_read = process->io_read_bytes;
    uint64_t last_write = process->io_write_bytes;
    uint64_t curr_time_ms = sys_monitor_info_->curr_time_ms;
    uint64_t time_delta_ms = curr_time_ms > process->io_last_scan_time_ms ? (
        curr_time_ms - process->io_last_scan_time_ms) : 0;

    const char* line;
    char* buf = buffer;
    while ((line = strsep(&buf, "\n")) != nullptr) {
        switch (line[0]) {
        case 'r':
            if (line[1] == 'c' && Util::wrap_strncmp(line+2, "har: ")) {
                process->io_read_char = strtoll(line+7, nullptr, 10);
            } else if (Util::wrap_strncmp(line+1, "ead_bytes: ")) {
                process->io_read_bytes = strtoll(line+12, nullptr, 10);
                process->io_rate_read_bps = time_delta_ms ? (
                    process->io_read_bytes - last_read) * 1000.0 / time_delta_ms : NAN;
            }
            break;
        case 'w':
            if (line[1] == 'c' && Util::wrap_strncmp(line+2, "har: ")) {
                process->io_write_char = strtoll(line+7, nullptr, 10);
            } else if (Util::wrap_strncmp(line+1, "rite_bytes: ")) {
                process->io_write_bytes = strtoll(line+13, nullptr, 10);
                process->io_rate_write_bps = time_delta_ms ? (
                    process->io_write_bytes - last_write) * 1000.0 / time_delta_ms : NAN;
            }
            break;
        case 's':
            if (line[4] == 'r' && Util::wrap_strncmp(line+1, "yscr: ")) {
                process->io_read_syscalls = strtoll(line+7, nullptr, 10);
            } else if (Util::wrap_strncmp(line+1, "yscw: ")) {
                process->io_write_syscalls = strtoll(line+7, nullptr, 10);
            }
            break;
        case 'c':
            if (Util::wrap_strncmp(line+1, "ancelled_write_bytes: ")) {
                process->io_cancelled_write_bytes = strtoll(line+23, nullptr, 10);
            }
        }
    }
    process->io_last_scan_time_ms = curr_time_ms;
}

int MonitorInfoCollection::get_task_statm_info(int proc_fd, std::shared_ptr<ProcessInfo> process) {
    FILE* statm_file = Util::fopenat(proc_fd, "statm", "r");
    if (!statm_file) {
        ERROR_LOG("fopenat statm failed, err: %s", strerror(errno));
        return -1;
    }
    uint64_t dummy, dummy2;
    int res = fscanf(statm_file, "%ld %ld %ld %ld %ld %ld %ld",
        &process->virtual_mem,
        &process->resident_mem,
        &process->shared_mem,
        &process->text_mem,
        &dummy,  // library (unused since Linux 2.6; always 0)
        &process->data_mem,
        &dummy2);  // dirty pages (unused since Linux 2.6; always 0)
    fclose(statm_file);
    if (res == 7) {
        process->virtual_mem *= page_size_kb_;
        process->resident_mem *= page_size_kb_;
    }
    return (res == 7) ? 0 : -1;
}

int MonitorInfoCollection::get_task_stat_info(int proc_fd, std::shared_ptr<ProcessInfo> process) {
    char buf[MAX_BYTES_ONCE_READ+1];
    ssize_t res = Util::read_file(proc_fd, "stat", buf, sizeof(buf));
    if (res < 0) {
        ERROR_LOG("read file stat failed, err: %s", strerror(errno));
        return -1;
    }
    int index = 1;
    char* endptr = nullptr;
    char* p_save = nullptr;
    char* p_token = strtok_r(buf, " ", &p_save);
    while (p_token != nullptr) {
        switch (index) {
        case 1:  // pid
            {
                int curr_pid = atoi(p_token);
                if (process->pid != curr_pid) {
                    ERROR_LOG("gathered pid: %d stat info, expeed pid: %d", curr_pid, process->pid);
                    return -2;
                }
            }
            break;
        case 2:  // command
            {
                size_t i = 0;
                for (; i < strlen(p_token) && i < MAX_COMMAND_LENGTH; i++) {
                    process->cmdline[i] = p_token[i];
                }
                process->cmdline[i] = '\0';
            }
            break;
        case 4:  // ppid
            process->ppid = atoi(p_token);
            break;
        case 14:  // utime
            {
                uint64_t utime = strtoull(p_token, &endptr, 10);
                if (*endptr == '\0') {
                    process->utime = adjust_time(utime);
                } else {
                    process->utime = 0;
                }
            }
            break;
        case 15:  // stime
            {
                uint64_t stime = strtoull(p_token, &endptr, 10);
                if (*endptr == '\0') {
                    process->stime = adjust_time(stime);
                } else {
                    process->stime = 0;
                }
            }
            break;
        case 16:  // cutime
            {
                uint64_t cutime = strtoull(p_token, &endptr, 10);
                if (*endptr != '\0') {
                    process->cutime = 0;
                } else {
                    process->cutime = adjust_time(cutime);
                }
            }
            break;
        case 17:  // cstime
            {
                uint64_t cstime = strtoull(p_token, &endptr, 10);
                if (*endptr != '\0') {
                    process->cstime = 0;
                } else {
                    process->cstime = adjust_time(cstime);
                }
            }
            // 提高性能，取完第 17 个位置的 cstime，就可以直接退出，不用在循环了
            return 0;
        }
        index++;
        p_token = strtok_r(nullptr, " ", &p_save);
    }

    /*
    // 判断 pid 是否有误
    int curr_pid = atoi(buf);
    if (process->pid != curr_pid) {
        ERROR_LOG("gathered pid: %d stat info, expeed pid: %d", curr_pid, process->pid);
        return -2;
    }
    // 截取字符串中的 command
    char* location = strchr(buf, ' ');
    if (!location) {
        ERROR_LOG("strchr buf: %s by ' ' failed, err: %s", buf, strerror(errno));
        return -3;
    }
    location += 2;
    char* end = strrchr(location, ')');
    if (!end) {
        ERROR_LOG("strrchr buf: %s by ')' failed, err: %s", location, strerror(errno));
        return -4;
    }
    // 复制 cmdline 信息
    size_t command_size = MINIMUM((end-location+1), sizeof(process->cmdline));
    size_t i = 0;
    for (; i < command_size-1 && location[i]; i++) {
        process->cmdline[i] = location[i];
    }
    process->cmdline[i] = '\0';
    location = end+2;
    // 跳过获取进程的状态
    location += 2;
    // 获取当前进程的父进程: ppid
    process->ppid = strtol(location, &location, 10);
    location += 1;
    // 跳过进程的 pgrp、session、tty_nr、tpgin、flags、minflt、cminflt、majflt、cmajflt
    // 一共跳过 9 个属性
    location += 9;
    // 获取 utime
    process->utime = adjust_time(strtoll(location, &location, 10));
    if (curr_pid == 20611) {
        std::cout << "procss utime 20611: " << process->utime  << std::endl;
    }
    location += 1;
    // 获取 stime
    process->stime = adjust_time(strtoll(location, &location, 10));
    location += 1;
    // 获取 cutime
    process->cutime = adjust_time(strtoll(location, &location, 10));
    location += 1;
    // 获取 cstime
    process->cstime = adjust_time(strtoll(location, &location, 10));
    // location += 1;
    // stat 文件中其他数据不再读取
    */
    return 0;
}

// int MonitorInfoCollection::get_task_cmdline_info(int proc_fd) {
//     char command[4096+1];  // linux 中 cmdline 最大的长度
//     ssize_t read_size = Util::read_file(proc_fd, "cmdline", command, sizeof(command));
//     if (read_size <= 0) {
//         ERROR_LOG("read file: cmdline failed, err: %s", strerror(errno));
//         return -1;
//     }
//     int token_end = 0;
//     int token_start = 0;
//     int last_char = 0;
//     bool arg_seq_null = false;
//     bool arg_seq_space = false;
//     for (size_t i = 0; i < read_size; i++) {
//         if (command[i] == '\0') {
//             command[i] = '\n';
//         } else {
//             if (token_end) arg_seq_null = true;
//             if (command[i] <= ' ') arg_seq_space = true;
//         }

//         if (command[i] == '\n') {
//             if (token_end == 0) token_end = i;
//         } else {
//             if (!token_end && command[i] == '/') token_start = i + 1;
//             last_char = i;
//         }
//     }
//     command[last_char + 1] = '\0';
//     if (!arg_seq_null && arg_seq_space) {

//     }
// }


std::shared_ptr<ProcessInfo> MonitorInfoCollection::get_process_info(
    uint64_t pid, bool* prev_existed) {
    auto iter = sys_monitor_info_->all_process_info_table.find(pid);
    if (iter != sys_monitor_info_->all_process_info_table.end()) {
        *prev_existed = true;
        return iter->second;
    } else {
        *prev_existed = false;
        auto process_info = std::make_shared<ProcessInfo>();
        process_info->pid = pid;
        return process_info;
    }
}
