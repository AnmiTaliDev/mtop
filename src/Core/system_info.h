#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <net/if.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "parser.h"

#define PROCESS_NAME_SIZE 64
#define USER_NAME_SIZE 33

typedef struct {
    int pid;
    char name[PROCESS_NAME_SIZE];
    char state;
    double cpu_percent;
    uint64_t memory_kb;
    char user[USER_NAME_SIZE];
    uid_t uid;
    bool is_kernel_thread;
    uint64_t cpu_time;
    uint64_t start_time;
} ProcessInfo;

typedef struct {
    char name[IF_NAMESIZE];
    uint64_t rx_bytes;
    uint64_t tx_bytes;
} NetworkStats;

typedef struct {
    double cpu_percent;
    uint64_t total_memory_kb;
    uint64_t used_memory_kb;
    double load_avg[3];
    int process_count;

    ProcessInfo *processes;
    size_t shown_count;
    size_t process_capacity;

    NetworkStats *interfaces;
    size_t interface_count;
    size_t interface_capacity;
} SystemStats;

typedef struct {
    int pid;
    uint64_t start_time;
    uint64_t cpu_time;
} ProcessSample;

typedef struct {
    uint64_t prev_total_time;
    uint64_t prev_idle_time;
    uint64_t total_time_delta;

    ProcessSample *samples;
    size_t sample_count;
} SystemInfo;

void systemInfoInit(SystemInfo *info, SystemStats *stats);
void systemInfoFree(SystemInfo *info, SystemStats *stats);
bool systemInfoUpdate(SystemInfo *info, SystemStats *stats,
                      const MtopConfig *config);

#endif
