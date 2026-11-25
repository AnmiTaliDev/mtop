#ifndef VIEW_H
#define VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "system_info.h"

#define QUERY_SIZE 64
#define MESSAGE_SIZE 128

typedef enum {
    MODE_NORMAL,
    MODE_HELP,
    MODE_SEARCH,
    MODE_CONFIRM
} Mode;

typedef struct {
    int pid;
    uint64_t start_time;
    char name[PROCESS_NAME_SIZE];
} Target;

typedef struct {
    Mode mode;

    size_t *indices;
    size_t count;
    size_t capacity;

    size_t cursor;
    size_t offset;
    size_t page;
    int selected_pid;

    char query[QUERY_SIZE];
    Target target;
    char message[MESSAGE_SIZE];
} View;

void viewInit(View *view);
void viewFree(View *view);
bool viewRefresh(View *view, const SystemStats *stats);
void viewMove(View *view, const SystemStats *stats, long delta);
void viewSelectFirst(View *view);
void viewSelectLast(View *view, const SystemStats *stats);
void viewScroll(View *view, size_t rows);
const ProcessInfo *viewProcess(const View *view, const SystemStats *stats,
                               size_t position);

#endif
