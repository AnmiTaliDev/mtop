#include "control.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "system_info.h"

void controlTerminate(int pid, uint64_t start_time, char *message, size_t size)
{
    uint64_t current_start_time;
    if (!systemInfoStartTime(pid, &current_start_time) ||
        current_start_time != start_time) {
        snprintf(message, size, "Process %d has exited", pid);
        return;
    }

    if (kill((pid_t)pid, SIGTERM) != 0) {
        snprintf(message, size, "Cannot signal %d: %s", pid, strerror(errno));
        return;
    }

    snprintf(message, size, "Sent SIGTERM to %d", pid);
}
