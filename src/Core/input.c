#include "input.h"

#include <string.h>

#include "control.h"
#include "terminal.h"

#define MAX_KEY_INTERVAL 10
#define KEY_CTRL_H 8
#define KEY_DELETE 127

static InputResult refresh(View *view, const SystemStats *stats)
{
    return viewRefresh(view, stats) ? INPUT_REDRAW : INPUT_FAILED;
}

static InputResult sortBy(MtopConfig *config, SystemStats *stats, View *view,
                          SortBy sort_by)
{
    config->sort_by = sort_by;
    systemInfoSort(stats, config);

    return refresh(view, stats);
}

static bool handleNavigation(int key, const SystemStats *stats, View *view)
{
    long page = view->page > 0 ? (long)view->page : 1;

    switch (key) {
    case KEY_UP:
        viewMove(view, stats, -1);
        return true;
    case KEY_DOWN:
        viewMove(view, stats, 1);
        return true;
    case KEY_PAGE_UP:
        viewMove(view, stats, -page);
        return true;
    case KEY_PAGE_DOWN:
        viewMove(view, stats, page);
        return true;
    case KEY_HOME:
        viewSelectFirst(view);
        return true;
    case KEY_END:
        viewSelectLast(view, stats);
        return true;
    default:
        return false;
    }
}

static InputResult clearSearch(View *view, const SystemStats *stats)
{
    view->mode = MODE_NORMAL;
    view->query[0] = '\0';

    return refresh(view, stats);
}

static void eraseCharacter(char *text)
{
    size_t length = strlen(text);
    while (length > 0) {
        length--;
        bool continuation = ((unsigned char)text[length] & 0xC0) == 0x80;
        text[length] = '\0';
        if (!continuation) {
            break;
        }
    }
}

static InputResult handleSearch(int key, const SystemStats *stats, View *view)
{
    if (key == KEY_ESCAPE) {
        return clearSearch(view, stats);
    }
    if (key == '\n' || key == '\r') {
        view->mode = MODE_NORMAL;
        return INPUT_REDRAW;
    }

    size_t length = strlen(view->query);
    if (key == KEY_DELETE || key == KEY_CTRL_H) {
        eraseCharacter(view->query);
    } else if (key >= ' ' && length + 1 < sizeof(view->query)) {
        view->query[length] = (char)key;
        view->query[length + 1] = '\0';
    } else {
        return INPUT_IGNORED;
    }
    viewSelectFirst(view);

    return refresh(view, stats);
}

static InputResult handleConfirm(int key, View *view)
{
    view->mode = MODE_NORMAL;
    if (key == 'y' || key == 'Y') {
        controlTerminate(view->target.pid, view->target.start_time,
                         view->message, sizeof(view->message));
    }

    return INPUT_REDRAW;
}

static InputResult requestTerminate(const SystemStats *stats, View *view)
{
    const ProcessInfo *process = viewProcess(view, stats, view->cursor);
    if (!process) {
        return INPUT_IGNORED;
    }

    view->selected_pid = process->pid;
    view->target = (Target){.pid = process->pid,
                            .start_time = process->start_time};
    memcpy(view->target.name, process->name, sizeof(view->target.name));
    view->mode = MODE_CONFIRM;

    return INPUT_REDRAW;
}

static InputResult handleNormal(int key, MtopConfig *config,
                                SystemStats *stats, View *view)
{
    switch (key) {
    case 'q':
    case 'Q':
        return INPUT_QUIT;
    case KEY_ESCAPE:
        return view->query[0] ? clearSearch(view, stats) : INPUT_QUIT;
    case '/':
        view->mode = MODE_SEARCH;
        return INPUT_REDRAW;
    case 'k':
    case 'K':
        return requestTerminate(stats, view);
    case 'm':
    case 'M':
        return sortBy(config, stats, view, SORT_MEMORY);
    case 'c':
    case 'C':
        return sortBy(config, stats, view, SORT_CPU);
    case 'p':
    case 'P':
        return sortBy(config, stats, view, SORT_PID);
    case 'n':
    case 'N':
        return sortBy(config, stats, view, SORT_NAME);
    case 'r':
    case 'R':
        config->reverse_sort = !config->reverse_sort;
        return sortBy(config, stats, view, config->sort_by);
    case 't':
    case 'T':
        config->show_network_stats = !config->show_network_stats;
        return INPUT_REDRAW;
    case '+':
    case '=':
        if (config->update_interval <= 1) {
            return INPUT_IGNORED;
        }
        config->update_interval--;
        return INPUT_REDRAW;
    case '-':
    case '_':
        if (config->update_interval >= MAX_KEY_INTERVAL) {
            return INPUT_IGNORED;
        }
        config->update_interval++;
        return INPUT_REDRAW;
    case 'h':
    case 'H':
    case '?':
        view->mode = MODE_HELP;
        return INPUT_REDRAW;
    default:
        return INPUT_IGNORED;
    }
}

InputResult inputHandle(int key, MtopConfig *config, SystemStats *stats,
                        View *view)
{
    view->message[0] = '\0';

    if (view->mode == MODE_HELP) {
        view->mode = MODE_NORMAL;
        return INPUT_REDRAW;
    }
    if (view->mode == MODE_CONFIRM) {
        return handleConfirm(key, view);
    }
    if (handleNavigation(key, stats, view)) {
        return INPUT_REDRAW;
    }
    if (view->mode == MODE_SEARCH) {
        return handleSearch(key, stats, view);
    }

    return handleNormal(key, config, stats, view);
}
