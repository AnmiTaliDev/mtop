#include "system_info.h"

#include <ctype.h>
#include <dirent.h>
#include <inttypes.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KTHREADD_PID 2
#define INITIAL_CAPACITY 16

static void *growArray(void *items, size_t *capacity, size_t item_size)
{
    size_t new_capacity = *capacity ? *capacity * 2 : INITIAL_CAPACITY;
    void *new_items = realloc(items, new_capacity * item_size);
    if (new_items) {
        *capacity = new_capacity;
    }

    return new_items;
}

static void readCpuStats(SystemInfo *info, SystemStats *stats)
{
    stats->cpu_percent = 0.0;
    info->total_time_delta = 0;

    FILE *file = fopen("/proc/stat", "r");
    if (!file) {
        return;
    }

    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
    int fields = fscanf(file,
                        "cpu %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64
                        " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64,
                        &user, &nice, &system, &idle, &iowait, &irq, &softirq,
                        &steal);
    fclose(file);
    if (fields != 8) {
        return;
    }

    uint64_t total_time = user + nice + system + idle + iowait + irq +
                          softirq + steal;
    uint64_t idle_time = idle + iowait;

    if (info->prev_total_time != 0 && total_time > info->prev_total_time) {
        info->total_time_delta = total_time - info->prev_total_time;

        uint64_t idle_delta = 0;
        if (idle_time > info->prev_idle_time) {
            idle_delta = idle_time - info->prev_idle_time;
        }
        if (idle_delta > info->total_time_delta) {
            idle_delta = info->total_time_delta;
        }

        stats->cpu_percent = 100.0 * (double)(info->total_time_delta -
                                              idle_delta) /
                             (double)info->total_time_delta;
    }

    info->prev_total_time = total_time;
    info->prev_idle_time = idle_time;
}

static void readMemoryStats(SystemStats *stats)
{
    stats->total_memory_kb = 0;
    stats->used_memory_kb = 0;
    stats->total_swap_kb = 0;
    stats->used_swap_kb = 0;

    FILE *file = fopen("/proc/meminfo", "r");
    if (!file) {
        return;
    }

    uint64_t free_kb = 0;
    uint64_t available_kb = 0;
    uint64_t free_swap_kb = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "MemTotal: %" SCNu64, &stats->total_memory_kb) == 1) {
            continue;
        }
        if (sscanf(line, "MemFree: %" SCNu64, &free_kb) == 1) {
            continue;
        }
        if (sscanf(line, "SwapTotal: %" SCNu64, &stats->total_swap_kb) == 1) {
            continue;
        }
        if (sscanf(line, "SwapFree: %" SCNu64, &free_swap_kb) == 1) {
            continue;
        }
        sscanf(line, "MemAvailable: %" SCNu64, &available_kb);
    }
    fclose(file);

    uint64_t unused_kb = available_kb ? available_kb : free_kb;
    if (stats->total_memory_kb >= unused_kb) {
        stats->used_memory_kb = stats->total_memory_kb - unused_kb;
    }
    if (stats->total_swap_kb >= free_swap_kb) {
        stats->used_swap_kb = stats->total_swap_kb - free_swap_kb;
    }
}

static void readLoadAverage(SystemStats *stats)
{
    stats->load_avg[0] = stats->load_avg[1] = stats->load_avg[2] = 0.0;

    FILE *file = fopen("/proc/loadavg", "r");
    if (!file) {
        return;
    }

    if (fscanf(file, "%lf %lf %lf", &stats->load_avg[0], &stats->load_avg[1],
               &stats->load_avg[2]) != 3) {
        stats->load_avg[0] = stats->load_avg[1] = stats->load_avg[2] = 0.0;
    }
    fclose(file);
}

static bool readProcessStat(int pid, ProcessInfo *process)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *file = fopen(path, "r");
    if (!file) {
        return false;
    }

    char buffer[1024];
    size_t length = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[length] = '\0';

    const char *name_start = strchr(buffer, '(');
    const char *name_end = strrchr(buffer, ')');
    if (!name_start || !name_end || name_end < name_start) {
        return false;
    }
    snprintf(process->name, sizeof(process->name), "%.*s",
             (int)(name_end - name_start - 1), name_start + 1);

    int parent_pid;
    uint64_t user_time, system_time;
    int64_t resident_pages;
    int fields = sscanf(name_end + 1,
                        " %c %d %*d %*d %*d %*d %*u %*u %*u %*u %*u"
                        " %" SCNu64 " %" SCNu64
                        " %*d %*d %*d %*d %*d %*d"
                        " %" SCNu64 " %*u %" SCNd64,
                        &process->state, &parent_pid, &user_time,
                        &system_time, &process->start_time, &resident_pages);
    if (fields != 6) {
        return false;
    }

    process->pid = pid;
    process->cpu_time = user_time + system_time;
    process->is_kernel_thread = pid == KTHREADD_PID ||
                                parent_pid == KTHREADD_PID;

    uint64_t page_size_kb = (uint64_t)sysconf(_SC_PAGESIZE) / 1024;
    process->memory_kb = resident_pages > 0
                             ? (uint64_t)resident_pages * page_size_kb
                             : 0;

    return true;
}

static bool readProcessUid(int pid, uid_t *uid)
{
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    FILE *file = fopen(path, "r");
    if (!file) {
        return false;
    }

    bool found = false;
    char line[256];
    while (!found && fgets(line, sizeof(line), file)) {
        unsigned int value;
        if (sscanf(line, "Uid: %u", &value) == 1) {
            *uid = (uid_t)value;
            found = true;
        }
    }
    fclose(file);

    return found;
}

static void readUserName(ProcessInfo *process)
{
    const struct passwd *entry = getpwuid(process->uid);
    if (entry) {
        snprintf(process->user, sizeof(process->user), "%s", entry->pw_name);
    } else {
        snprintf(process->user, sizeof(process->user), "%u",
                 (unsigned int)process->uid);
    }
}

static int compareSamples(const void *left, const void *right)
{
    const ProcessSample *a = left;
    const ProcessSample *b = right;

    return (a->pid > b->pid) - (a->pid < b->pid);
}

static double processCpuPercent(const SystemInfo *info,
                                const ProcessInfo *process, long cpu_count)
{
    if (info->total_time_delta == 0 || info->sample_count == 0) {
        return 0.0;
    }

    ProcessSample key = {.pid = process->pid};
    const ProcessSample *previous = bsearch(&key, info->samples,
                                            info->sample_count, sizeof(key),
                                            compareSamples);
    if (!previous || previous->start_time != process->start_time ||
        process->cpu_time < previous->cpu_time) {
        return 0.0;
    }

    double percent = 100.0 * (double)(process->cpu_time - previous->cpu_time) *
                     (double)cpu_count / (double)info->total_time_delta;
    double limit = 100.0 * (double)cpu_count;

    return percent < limit ? percent : limit;
}

static int parsePid(const char *name)
{
    char *end = NULL;
    long pid = strtol(name, &end, 10);
    if (!isdigit((unsigned char)name[0]) || *end != '\0' || pid > INT_MAX) {
        return 0;
    }

    return (int)pid;
}

static bool readProcesses(const SystemInfo *info, SystemStats *stats)
{
    stats->listed_count = 0;
    stats->process_count = 0;

    DIR *directory = opendir("/proc");
    if (!directory) {
        return true;
    }

    long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count <= 0) {
        cpu_count = 1;
    }

    bool success = true;
    const struct dirent *entry;
    while ((entry = readdir(directory))) {
        int pid = parsePid(entry->d_name);
        ProcessInfo process = {0};
        if (pid == 0 || !readProcessStat(pid, &process) ||
            !readProcessUid(pid, &process.uid)) {
            continue;
        }
        readUserName(&process);
        process.cpu_percent = processCpuPercent(info, &process, cpu_count);

        if (stats->listed_count == stats->process_capacity) {
            ProcessInfo *processes = growArray(stats->processes,
                                               &stats->process_capacity,
                                               sizeof(*processes));
            if (!processes) {
                success = false;
                break;
            }
            stats->processes = processes;
        }
        stats->processes[stats->listed_count++] = process;
    }
    closedir(directory);

    stats->process_count = (int)stats->listed_count;

    return success;
}

static bool saveSamples(SystemInfo *info, const SystemStats *stats)
{
    info->sample_count = 0;
    if (stats->listed_count == 0) {
        return true;
    }

    ProcessSample *samples = realloc(info->samples,
                                     stats->listed_count * sizeof(*samples));
    if (!samples) {
        return false;
    }
    info->samples = samples;

    for (size_t i = 0; i < stats->listed_count; i++) {
        samples[i] = (ProcessSample){
            .pid = stats->processes[i].pid,
            .start_time = stats->processes[i].start_time,
            .cpu_time = stats->processes[i].cpu_time,
        };
    }
    info->sample_count = stats->listed_count;
    qsort(samples, info->sample_count, sizeof(*samples), compareSamples);

    return true;
}

static bool listContains(const StringList *list, const char *value)
{
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->items[i], value) == 0) {
            return true;
        }
    }

    return false;
}

static bool shouldShowProcess(const ProcessInfo *process,
                              const MtopConfig *config)
{
    if (process->is_kernel_thread && !config->show_kernel_threads) {
        return false;
    }

    for (size_t i = 0; i < config->hide_processes.count; i++) {
        if (strstr(process->name, config->hide_processes.items[i])) {
            return false;
        }
    }

    return config->show_only_users.count == 0 ||
           listContains(&config->show_only_users, process->user);
}

static void filterProcesses(SystemStats *stats, const MtopConfig *config)
{
    size_t kept = 0;
    for (size_t i = 0; i < stats->listed_count; i++) {
        if (shouldShowProcess(&stats->processes[i], config)) {
            stats->processes[kept++] = stats->processes[i];
        }
    }
    stats->listed_count = kept;
}

static int compareByPid(const void *left, const void *right)
{
    const ProcessInfo *a = left;
    const ProcessInfo *b = right;

    return (a->pid > b->pid) - (a->pid < b->pid);
}

static int compareByMemory(const void *left, const void *right)
{
    const ProcessInfo *a = left;
    const ProcessInfo *b = right;
    int result = (a->memory_kb < b->memory_kb) -
                 (a->memory_kb > b->memory_kb);

    return result ? result : compareByPid(left, right);
}

static int compareByCpu(const void *left, const void *right)
{
    const ProcessInfo *a = left;
    const ProcessInfo *b = right;
    int result = (a->cpu_percent < b->cpu_percent) -
                 (a->cpu_percent > b->cpu_percent);

    return result ? result : compareByPid(left, right);
}

static int compareByName(const void *left, const void *right)
{
    const ProcessInfo *a = left;
    const ProcessInfo *b = right;
    int result = strcmp(a->name, b->name);

    return result ? result : compareByPid(left, right);
}

void systemInfoInit(SystemInfo *info, SystemStats *stats)
{
    *info = (SystemInfo){0};
    *stats = (SystemStats){0};
}

void systemInfoFree(SystemInfo *info, SystemStats *stats)
{
    free(info->samples);
    free(stats->processes);
    systemInfoInit(info, stats);
}

bool systemInfoSample(SystemInfo *info, SystemStats *stats,
                      const MtopConfig *config)
{
    readCpuStats(info, stats);
    readMemoryStats(stats);
    readLoadAverage(stats);
    networkUpdate(&info->network_sample, &stats->network);

    if (info->total_time_delta > 0) {
        historyPush(&stats->cpu_history, stats->cpu_percent);
    }
    if (stats->network.has_rate) {
        historyPush(&stats->rx_history, stats->network.rx_rate);
        historyPush(&stats->tx_history, stats->network.tx_rate);
    }

    if (!readProcesses(info, stats) || !saveSamples(info, stats)) {
        return false;
    }

    filterProcesses(stats, config);
    systemInfoSort(stats, config);

    return true;
}

void systemInfoSort(SystemStats *stats, const MtopConfig *config)
{
    if (stats->listed_count == 0) {
        return;
    }

    int (*compare)(const void *, const void *) = compareByMemory;
    switch (config->sort_by) {
    case SORT_MEMORY:
        compare = compareByMemory;
        break;
    case SORT_CPU:
        compare = compareByCpu;
        break;
    case SORT_PID:
        compare = compareByPid;
        break;
    case SORT_NAME:
        compare = compareByName;
        break;
    }
    qsort(stats->processes, stats->listed_count, sizeof(*stats->processes),
          compare);

    if (!config->reverse_sort) {
        return;
    }
    for (size_t i = 0, j = stats->listed_count - 1; i < j; i++, j--) {
        ProcessInfo swap = stats->processes[i];
        stats->processes[i] = stats->processes[j];
        stats->processes[j] = swap;
    }
}

bool systemInfoStartTime(int pid, uint64_t *start_time)
{
    ProcessInfo process;
    if (!readProcessStat(pid, &process)) {
        return false;
    }
    *start_time = process.start_time;

    return true;
}
