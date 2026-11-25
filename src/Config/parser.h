#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    SORT_MEMORY,
    SORT_CPU,
    SORT_PID,
    SORT_NAME
} SortBy;

typedef struct {
    char **items;
    size_t count;
} StringList;

typedef struct {
    int update_interval;
    int max_processes;
    bool show_colors;
    bool show_load_avg;
    bool show_memory_bar;
    bool show_cpu_bar;
    bool show_network_stats;

    SortBy sort_by;
    bool reverse_sort;

    int progress_bar_width;
    bool show_process_state;
    bool show_process_user;

    StringList hide_processes;
    StringList show_only_users;
    bool show_kernel_threads;
} MtopConfig;

typedef enum {
    ARGS_RUN,
    ARGS_EXIT,
    ARGS_ERROR
} ArgsResult;

void configInit(MtopConfig *config);
void configFree(MtopConfig *config);
bool configLoad(MtopConfig *config, const char *path);
void configLoadDefault(MtopConfig *config);
ArgsResult configParseArgs(MtopConfig *config, int argc, char *argv[]);

#endif
