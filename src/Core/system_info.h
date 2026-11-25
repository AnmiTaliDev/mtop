#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include "history.h"
#include "network.h"
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
    double cpu_percent;
    uint64_t total_memory_kb;
    uint64_t used_memory_kb;
    uint64_t total_swap_kb;
    uint64_t used_swap_kb;
    double load_avg[3];
    int process_count;

    ProcessInfo *processes;
    size_t listed_count;
    size_t process_capacity;

    NetworkStats network;

    History cpu_history;
    History rx_history;
    History tx_history;
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

    NetworkSample network_sample;
} SystemInfo;

void systemInfoInit(SystemInfo *info, SystemStats *stats);
void systemInfoFree(SystemInfo *info, SystemStats *stats);
bool systemInfoSample(SystemInfo *info, SystemStats *stats,
                      const MtopConfig *config);
void systemInfoSort(SystemStats *stats, const MtopConfig *config);

#endif
