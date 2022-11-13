#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

void Util::fatal_log(const char* note) {
    const char* sys_msg = strerror(errno);
    fprintf(stderr, "%s: %s\n", note, sys_msg);
    exit(-1);
}
