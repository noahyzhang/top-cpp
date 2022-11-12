#include <errno.h>
#include <string.h>
#include "common.h"

void Util::fatal_log(const char* note) {
    const char* sys_msg = strerror(errno);
}