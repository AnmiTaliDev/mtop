#include "view.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

static bool matchesQuery(const char *name, const char *query)
{
    size_t length = strlen(query);
    if (length == 0) {
        return true;
    }

    for (; *name; name++) {
        if (strncasecmp(name, query, length) == 0) {
            return true;
        }
    }

    return false;
}

static void selectPosition(View *view, const SystemStats *stats,
                           size_t position)
{
    view->cursor = position;
    view->selected_pid = viewProcess(view, stats, position)->pid;
}

void viewInit(View *view)
{
    *view = (View){0};
}

void viewFree(View *view)
{
    free(view->indices);
    viewInit(view);
}

bool viewRefresh(View *view, const SystemStats *stats)
{
    if (stats->listed_count > view->capacity) {
        size_t *indices = realloc(view->indices,
                                  stats->listed_count * sizeof(*indices));
        if (!indices) {
            return false;
        }
        view->indices = indices;
        view->capacity = stats->listed_count;
    }

    view->count = 0;
    for (size_t i = 0; i < stats->listed_count; i++) {
        if (matchesQuery(stats->processes[i].name, view->query)) {
            view->indices[view->count++] = i;
        }
    }

    if (view->selected_pid == 0 || view->count == 0) {
        view->cursor = 0;
        return true;
    }

    for (size_t i = 0; i < view->count; i++) {
        if (viewProcess(view, stats, i)->pid == view->selected_pid) {
            view->cursor = i;
            return true;
        }
    }
    selectPosition(view, stats,
                   view->cursor < view->count ? view->cursor
                                              : view->count - 1);

    return true;
}

void viewMove(View *view, const SystemStats *stats, long delta)
{
    if (view->count == 0) {
        return;
    }

    long position = (long)view->cursor + delta;
    if (position < 0) {
        position = 0;
    } else if (position >= (long)view->count) {
        position = (long)view->count - 1;
    }
    selectPosition(view, stats, (size_t)position);
}

void viewSelectFirst(View *view)
{
    view->selected_pid = 0;
    view->cursor = 0;
    view->offset = 0;
}

void viewSelectLast(View *view, const SystemStats *stats)
{
    if (view->count > 0) {
        selectPosition(view, stats, view->count - 1);
    }
}

void viewScroll(View *view, size_t rows)
{
    view->page = rows;

    if (view->cursor < view->offset) {
        view->offset = view->cursor;
    } else if (view->cursor >= view->offset + rows) {
        view->offset = view->cursor - rows + 1;
    }
    if (view->offset + rows > view->count) {
        view->offset = view->count > rows ? view->count - rows : 0;
    }
}

const ProcessInfo *viewProcess(const View *view, const SystemStats *stats,
                               size_t position)
{
    if (position >= view->count) {
        return NULL;
    }

    return &stats->processes[view->indices[position]];
}
