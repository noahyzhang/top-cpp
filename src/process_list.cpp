#include "process_list.h"

void ProcessListCollection::get_process_list_monitor_info() {

}

void ProcessListCollection::get_mem_monitor_info() {
    FILE* file = fopen(PROC_MEMINFO_FILE, "r");
    if (!file) {
        
    }
}