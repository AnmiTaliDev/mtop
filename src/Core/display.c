#include "display.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "theme.h"

#define MIN_WIDTH 60
#define MIN_HEIGHT 16
#define DEFAULT_WIDTH 80
#define DEFAULT_HEIGHT 24

#define SUMMARY_HEIGHT 8
#define NETWORK_HEIGHT 3
#define MEMORY_WIDTH 30
#define HELP_WIDTH 48
#define CONFIRM_WIDTH 52

#define OUTPUT_BUFFER_SIZE (1 << 16)
#define TEXT_SIZE 256
#define BYTES_SIZE 32
#define USER_WIDTH 10
#define GRAPH_LEVELS 8
#define MIN_NETWORK_SCALE 1024.0

static void drawText(Screen *screen, int x, int y, int max_width, Style style,
                     const char *format, ...)
    __attribute__((format(printf, 6, 7)));

static void drawText(Screen *screen, int x, int y, int max_width, Style style,
                     const char *format, ...)
{
    char text[TEXT_SIZE];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);

    screenText(screen, x, y, max_width, text, style);
}

static void formatBytes(double bytes, char *buffer, size_t size)
{
    static const char units[] = "BKMGT";
    size_t unit = 0;

    while (bytes >= 1024.0 && unit + 2 < sizeof(units)) {
        bytes /= 1024.0;
        unit++;
    }

    snprintf(buffer, size, "%.1f%c", bytes, units[unit]);
}

static Style usageStyle(double fraction)
{
    if (fraction > 0.75) {
        return STYLE_HIGH;
    }
    if (fraction > 0.5) {
        return STYLE_MEDIUM;
    }

    return STYLE_LOW;
}

static void drawPanel(Screen *screen, const MtopConfig *config, int x, int y,
                      int width, int height, const char *title,
                      const char *detail)
{
    screenBox(screen, x, y, width, height, !config->show_colors);
    drawText(screen, x + 2, y, width - 4, STYLE_TITLE, " %s ", title);

    int length = (int)strlen(detail);
    if (length > 0 && length + (int)strlen(title) + 8 <= width) {
        drawText(screen, x + width - length - 4, y, length + 2, STYLE_HEADER,
                 " %s ", detail);
    }
}

static void drawGraph(Screen *screen, const MtopConfig *config, int x, int y,
                      int width, int height, const History *history,
                      double maximum, bool color_by_height)
{
    static const char *const blocks[GRAPH_LEVELS] = {
        "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    static const char *const ascii[GRAPH_LEVELS] = {
        ".", ":", "-", "=", "+", "*", "%", "#"};
    const char *const *glyphs = config->show_colors ? blocks : ascii;

    for (int age = 0; age < width && (size_t)age < history->count; age++) {
        double value = historyGet(history, (size_t)age);
        int filled = (int)(value / maximum * height * GRAPH_LEVELS + 0.5);
        if (filled < 1) {
            filled = 1;
        }

        for (int level = 0; level < height; level++) {
            int part = filled - level * GRAPH_LEVELS;
            if (part <= 0) {
                break;
            }
            if (part > GRAPH_LEVELS) {
                part = GRAPH_LEVELS;
            }
            Style style = color_by_height
                              ? usageStyle((double)(level + 1) / height)
                              : STYLE_ACCENT;
            screenPut(screen, x + width - 1 - age, y + height - 1 - level,
                      glyphs[part - 1], style);
        }
    }
}

static void drawCpuPanel(Screen *screen, const MtopConfig *config,
                         const SystemStats *stats, int width)
{
    char usage[BYTES_SIZE];
    snprintf(usage, sizeof(usage), "%.1f%%", stats->cpu_percent);
    drawPanel(screen, config, 0, 0, width, SUMMARY_HEIGHT, "cpu", usage);

    int rows = SUMMARY_HEIGHT - 2;
    if (config->show_load_avg) {
        rows--;
        drawText(screen, 2, SUMMARY_HEIGHT - 2, width - 4, STYLE_MUTED,
                 "load %.2f %.2f %.2f", stats->load_avg[0],
                 stats->load_avg[1], stats->load_avg[2]);
    }
    if (config->show_cpu_bar) {
        drawGraph(screen, config, 2, 1, width - 4, rows, &stats->cpu_history,
                  100.0, true);
    }
}

static void drawBar(Screen *screen, const MtopConfig *config, int x, int y,
                    int width, double fraction)
{
    const char *full = config->show_colors ? "■" : "#";
    const char *empty = config->show_colors ? "□" : "-";
    int filled = (int)(fraction * width + 0.5);
    Style style = usageStyle(fraction);

    for (int i = 0; i < width; i++) {
        screenPut(screen, x + i, y, i < filled ? full : empty,
                  i < filled ? style : STYLE_MUTED);
    }
}

static void drawUsage(Screen *screen, const MtopConfig *config, int x, int y,
                      int width, const char *label, uint64_t used_kb,
                      uint64_t total_kb)
{
    if (total_kb == 0) {
        drawText(screen, x, y, width, STYLE_MUTED, "%-5s none", label);
        return;
    }

    char used[BYTES_SIZE];
    char total[BYTES_SIZE];
    formatBytes((double)used_kb * 1024.0, used, sizeof(used));
    formatBytes((double)total_kb * 1024.0, total, sizeof(total));
    drawText(screen, x, y, width, STYLE_DEFAULT, "%-5s %s / %s", label, used,
             total);

    double fraction = (double)used_kb / (double)total_kb;
    int bar_width = width - 5;
    if (config->show_memory_bar) {
        drawBar(screen, config, x, y + 1, bar_width, fraction);
    }
    drawText(screen, x + bar_width + 1, y + 1, 4, STYLE_HEADER, "%3.0f%%",
             100.0 * fraction);
}

static void drawMemoryPanel(Screen *screen, const MtopConfig *config,
                            const SystemStats *stats, int x)
{
    drawPanel(screen, config, x, 0, MEMORY_WIDTH, SUMMARY_HEIGHT, "memory",
              "");
    drawUsage(screen, config, x + 2, 1, MEMORY_WIDTH - 4, "used",
              stats->used_memory_kb, stats->total_memory_kb);
    drawUsage(screen, config, x + 2, 4, MEMORY_WIDTH - 4, "swap",
              stats->used_swap_kb, stats->total_swap_kb);
}

static double visibleMaximum(const History *history, int width)
{
    double maximum = MIN_NETWORK_SCALE;
    for (int age = 0; age < width && (size_t)age < history->count; age++) {
        double value = historyGet(history, (size_t)age);
        if (value > maximum) {
            maximum = value;
        }
    }

    return maximum;
}

static void drawTraffic(Screen *screen, const MtopConfig *config, int x, int y,
                        int width, const char *label, double rate,
                        const History *history)
{
    char bytes[BYTES_SIZE];
    formatBytes(rate, bytes, sizeof(bytes));

    char text[TEXT_SIZE];
    snprintf(text, sizeof(text), "%-4s %9s/s ", label, bytes);
    int used = screenText(screen, x, y, width, text, STYLE_DEFAULT);

    int graph_width = width - used;
    drawGraph(screen, config, x + used, y, graph_width, 1, history,
              visibleMaximum(history, graph_width), false);
}

static void drawNetworkPanel(Screen *screen, const MtopConfig *config,
                             const SystemStats *stats, int y)
{
    char title[TEXT_SIZE];
    snprintf(title, sizeof(title), "network %s", stats->network.name);
    drawPanel(screen, config, 0, y, screen->width, NETWORK_HEIGHT, title, "");

    int half = (screen->width - 4) / 2;
    drawTraffic(screen, config, 2, y + 1, half - 1, "down",
                stats->network.rx_rate, &stats->rx_history);
    drawTraffic(screen, config, 2 + half + 1, y + 1, half - 1, "up",
                stats->network.tx_rate, &stats->tx_history);
}

static Style stateStyle(char state)
{
    switch (state) {
    case 'R':
        return STYLE_LOW;
    case 'D':
        return STYLE_MEDIUM;
    case 'Z':
        return STYLE_HIGH;
    default:
        return STYLE_MUTED;
    }
}

static void drawProcess(Screen *screen, const MtopConfig *config,
                        const ProcessInfo *process, int y, bool selected)
{
    if (selected) {
        for (int column = 1; column < screen->width - 1; column++) {
            screenPut(screen, column, y, " ", STYLE_SELECTED);
        }
        screenPut(screen, 1, y, ">", STYLE_SELECTED);
    }

    int x = 2;
    drawText(screen, x, y, 7, selected ? STYLE_SELECTED : STYLE_MUTED, "%7d",
             process->pid);
    x += 8;

    if (config->show_process_user) {
        screenText(screen, x, y, USER_WIDTH, process->user,
                   selected ? STYLE_SELECTED : STYLE_ACCENT);
    }
    x += USER_WIDTH + 1;

    if (config->show_process_state) {
        char state[2] = {process->state, '\0'};
        screenText(screen, x, y, 1, state,
                   selected ? STYLE_SELECTED : stateStyle(process->state));
    }
    x += 2;

    drawText(screen, x, y, 6,
             selected ? STYLE_SELECTED
                      : usageStyle(process->cpu_percent / 100.0),
             "%6.1f", process->cpu_percent);
    x += 7;

    char memory[BYTES_SIZE];
    formatBytes((double)process->memory_kb * 1024.0, memory, sizeof(memory));
    drawText(screen, x, y, 8, selected ? STYLE_SELECTED : STYLE_DEFAULT, "%8s",
             memory);
    x += 10;

    screenText(screen, x, y, screen->width - x - 2, process->name,
               selected ? STYLE_SELECTED : STYLE_HEADER);
}

static void drawProcessPanel(Screen *screen, const MtopConfig *config,
                             const SystemStats *stats, View *view, int y,
                             int height)
{
    static const char *const sort_names[] = {
        [SORT_MEMORY] = "memory",
        [SORT_CPU] = "cpu",
        [SORT_PID] = "pid",
        [SORT_NAME] = "name",
    };

    char title[TEXT_SIZE];
    char detail[TEXT_SIZE];
    if (view->query[0]) {
        snprintf(title, sizeof(title), "processes %zu/%d", view->count,
                 stats->process_count);
    } else {
        snprintf(title, sizeof(title), "processes %d", stats->process_count);
    }
    snprintf(detail, sizeof(detail), "sort: %s%s",
             sort_names[config->sort_by],
             config->reverse_sort ? " reversed" : "");
    drawPanel(screen, config, 0, y, screen->width, height, title, detail);

    drawText(screen, 2, y + 1, screen->width - 4, STYLE_TITLE,
             "%7s %-*s %s %6s %8s  %s", "PID", USER_WIDTH, "USER", "S",
             "CPU%", "MEM", "NAME");

    size_t rows = (size_t)(height - 3);
    viewScroll(view, rows);
    for (size_t row = 0; row < rows; row++) {
        size_t position = view->offset + row;
        const ProcessInfo *process = viewProcess(view, stats, position);
        if (!process) {
            break;
        }
        drawProcess(screen, config, process, y + 2 + (int)row,
                    position == view->cursor);
    }
}

static void drawFooter(Screen *screen, const MtopConfig *config,
                       const View *view)
{
    int y = screen->height - 1;
    int width = screen->width - 6;

    if (view->mode == MODE_SEARCH) {
        drawText(screen, 1, y, width, STYLE_HEADER,
                 "find: %s_  (Enter keep, Esc clear)", view->query);
    } else if (view->message[0]) {
        screenText(screen, 1, y, width, view->message, STYLE_HEADER);
    } else if (view->query[0]) {
        drawText(screen, 1, y, width, STYLE_HEADER,
                 "find: %s  (/ edit, Esc clear)", view->query);
    } else {
        screenText(screen, 1, y, width,
                   "q quit  / find  k term  m/c/p/n sort  r rev  h help",
                   STYLE_MUTED);
    }
    drawText(screen, screen->width - 4, y, 4, STYLE_HEADER, "%2ds",
             config->update_interval);
}

static void drawDialog(Screen *screen, const MtopConfig *config,
                       const char *title, int width,
                       const char *const *lines, int count)
{
    int height = count + 2;
    int x = (screen->width - width) / 2;
    int y = (screen->height - height) / 2;

    for (int row = y; row < y + height; row++) {
        for (int column = x; column < x + width; column++) {
            screenPut(screen, column, row, " ", STYLE_DEFAULT);
        }
    }
    drawPanel(screen, config, x, y, width, height, title, "");
    for (int i = 0; i < count; i++) {
        screenText(screen, x + 3, y + 1 + i, width - 6, lines[i],
                   STYLE_DEFAULT);
    }
}

static void drawHelp(Screen *screen, const MtopConfig *config)
{
    static const char *const lines[] = {
        "q            quit",
        "Up, Down     move selection",
        "PgUp, PgDn   move one page",
        "Home, End    first or last process",
        "/            find by name",
        "Esc          clear search, or quit",
        "k            send SIGTERM to selection",
        "m c p n      sort: memory, cpu, pid, name",
        "r            reverse sort order",
        "t            toggle network panel",
        "+ -          shorter or longer interval",
        "",
        "Press any key to close",
    };

    drawDialog(screen, config, "help", HELP_WIDTH, lines,
               (int)(sizeof(lines) / sizeof(lines[0])));
}

static void drawConfirm(Screen *screen, const MtopConfig *config,
                        const Target *target)
{
    char question[TEXT_SIZE];
    snprintf(question, sizeof(question), "Send SIGTERM to PID %d?",
             target->pid);
    const char *const lines[] = {
        question,
        target->name,
        "",
        "y confirm, any other key cancels",
    };

    drawDialog(screen, config, "terminate", CONFIRM_WIDTH, lines,
               (int)(sizeof(lines) / sizeof(lines[0])));
}

static void readTerminalSize(int *width, int *height)
{
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0 &&
        size.ws_row > 0) {
        *width = size.ws_col;
        *height = size.ws_row;
    } else {
        *width = DEFAULT_WIDTH;
        *height = DEFAULT_HEIGHT;
    }
}

void displayBegin(Display *display)
{
    static char output_buffer[OUTPUT_BUFFER_SIZE];

    *display = (Display){0};
    setvbuf(stdout, output_buffer, _IOFBF, sizeof(output_buffer));
    fputs("\033[?1049h\033[?25l\033[2J", stdout);
    fflush(stdout);
}

void displayEnd(Display *display)
{
    fputs("\033[0m\033[?25h\033[?1049l", stdout);
    fflush(stdout);
    screenFree(&display->screen);
}

bool displayDraw(Display *display, const MtopConfig *config,
                 const SystemStats *stats, View *view)
{
    Screen *screen = &display->screen;
    const char *const *styles =
        config->show_colors ? themeStyles(config->theme) : NULL;
    int width, height;
    readTerminalSize(&width, &height);
    if (width != screen->width || height != screen->height) {
        fputs("\033[0m\033[2J", stdout);
    }
    if (!screenReset(screen, width, height)) {
        return false;
    }

    if (width < MIN_WIDTH || height < MIN_HEIGHT) {
        screenText(screen, 0, 0, width, "Terminal too small", STYLE_HEADER);
        drawText(screen, 0, 1, width, STYLE_MUTED, "Need %dx%d", MIN_WIDTH,
                 MIN_HEIGHT);
        screenFlush(screen, styles);
        return true;
    }

    drawCpuPanel(screen, config, stats, width - MEMORY_WIDTH);
    drawMemoryPanel(screen, config, stats, width - MEMORY_WIDTH);

    int y = SUMMARY_HEIGHT;
    if (config->show_network_stats && stats->network.present) {
        drawNetworkPanel(screen, config, stats, y);
        y += NETWORK_HEIGHT;
    }
    drawProcessPanel(screen, config, stats, view, y, height - y - 1);
    drawFooter(screen, config, view);

    if (view->mode == MODE_HELP) {
        drawHelp(screen, config);
    } else if (view->mode == MODE_CONFIRM) {
        drawConfirm(screen, config, &view->target);
    }
    screenFlush(screen, styles);

    return true;
}
