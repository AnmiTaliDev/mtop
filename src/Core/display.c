#include "display.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define NAME_WIDTH 18
#define USER_WIDTH 12
#define BYTES_SIZE 32

#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_WHITE "\033[1;37m"
#define COLOR_GRAY "\033[1;90m"

static const char *color(const MtopConfig *config, const char *code)
{
    return config->show_colors ? code : "";
}

static void formatBytes(uint64_t bytes, char *buffer, size_t size)
{
    static const char *const units[] = {"B", "KB", "MB", "GB", "TB"};
    size_t unit = 0;
    double value = (double)bytes;

    while (value >= 1024.0 && unit + 1 < sizeof(units) / sizeof(units[0])) {
        value /= 1024.0;
        unit++;
    }

    snprintf(buffer, size, "%.1f%s", value, units[unit]);
}

static void formatCell(const char *text, int width, char *buffer, size_t size)
{
    if (strlen(text) > (size_t)width) {
        snprintf(buffer, size, "%.*s...", width - 3, text);
    } else {
        snprintf(buffer, size, "%s", text);
    }

    for (char *c = buffer; *c; c++) {
        if (iscntrl((unsigned char)*c)) {
            *c = '?';
        }
    }
}

static void clearScreen(const MtopConfig *config)
{
    if (config->show_colors) {
        fputs("\033[2J\033[H", stdout);
        return;
    }

    for (int i = 0; i < 50; i++) {
        putchar('\n');
    }
}

static void printHeader(const MtopConfig *config)
{
    if (config->show_colors) {
        fputs(COLOR_CYAN
              "╭─────────────────────────────────────────────────────────────────────────────╮\n"
              "│                              " COLOR_MAGENTA "mtop" COLOR_CYAN
              " - Modern Top                              │\n"
              "╰─────────────────────────────────────────────────────────────────────────────╯"
              COLOR_RESET "\n",
              stdout);
    } else {
        fputs("===============================================================================\n"
              "                              mtop - Modern Top                              \n"
              "===============================================================================\n",
              stdout);
    }
}

static void printBar(const MtopConfig *config, double percent)
{
    int width = config->progress_bar_width;
    int filled = (int)(percent / 100.0 * width);
    const char *full = config->show_colors ? "█" : "#";
    const char *empty = config->show_colors ? "░" : "-";

    printf("%s[", color(config, COLOR_GREEN));
    for (int i = 0; i < width; i++) {
        fputs(i < filled ? full : empty, stdout);
    }
    printf("]%s", color(config, COLOR_RESET));
}

static void printUsage(const MtopConfig *config, const char *label,
                       double percent, bool show_bar)
{
    printf("%s: ", label);
    if (show_bar) {
        printBar(config, percent);
        putchar(' ');
    }
    printf("%.1f%%", percent);
}

static void printNetworkStats(const MtopConfig *config,
                              const SystemStats *stats)
{
    printf("%sNetwork: %s", color(config, COLOR_YELLOW),
           color(config, COLOR_CYAN));

    for (size_t i = 0; i < stats->interface_count; i++) {
        const NetworkStats *interface = &stats->interfaces[i];
        char received[BYTES_SIZE];
        char sent[BYTES_SIZE];
        formatBytes(interface->rx_bytes, received, sizeof(received));
        formatBytes(interface->tx_bytes, sent, sizeof(sent));

        printf("%s%s RX:%s TX:%s", i > 0 ? " | " : "", interface->name,
               received, sent);
    }

    printf("%s\n", color(config, COLOR_RESET));
}

static void printSystemStats(const MtopConfig *config,
                             const SystemStats *stats)
{
    fputs(color(config, COLOR_YELLOW), stdout);
    printUsage(config, "CPU", stats->cpu_percent, config->show_cpu_bar);
    putchar('\n');

    double memory_percent = 0.0;
    if (stats->total_memory_kb > 0) {
        memory_percent = 100.0 * (double)stats->used_memory_kb /
                         (double)stats->total_memory_kb;
    }
    char used[BYTES_SIZE];
    char total[BYTES_SIZE];
    formatBytes(stats->used_memory_kb * 1024, used, sizeof(used));
    formatBytes(stats->total_memory_kb * 1024, total, sizeof(total));
    printUsage(config, "MEM", memory_percent, config->show_memory_bar);
    printf(" (%s/%s)\n", used, total);

    if (config->show_load_avg) {
        printf("Load: %s%.2f %.2f %.2f%s", color(config, COLOR_GREEN),
               stats->load_avg[0], stats->load_avg[1], stats->load_avg[2],
               color(config, COLOR_RESET));
    }
    printf("  Processes: %s%d%s\n", color(config, COLOR_GREEN),
           stats->process_count, color(config, COLOR_RESET));

    if (config->show_network_stats && stats->interface_count > 0) {
        printNetworkStats(config, stats);
    }

    putchar('\n');
}

static const char *stateColor(char state)
{
    switch (state) {
    case 'Z':
        return COLOR_RED;
    case 'D':
        return COLOR_YELLOW;
    default:
        return COLOR_GREEN;
    }
}

static void printProcess(const MtopConfig *config, const ProcessInfo *process)
{
    const char *separator = config->show_colors ? " │ " : " | ";
    const char *reset = color(config, COLOR_RESET);

    char name[PROCESS_NAME_SIZE];
    formatCell(process->name, NAME_WIDTH, name, sizeof(name));

    char user[USER_NAME_SIZE] = "";
    if (config->show_process_user) {
        formatCell(process->user, USER_WIDTH, user, sizeof(user));
    }

    char state[2] = "";
    if (config->show_process_state) {
        state[0] = process->state;
    }

    char memory[BYTES_SIZE];
    formatBytes(process->memory_kb * 1024, memory, sizeof(memory));

    printf("%s%7d%s", config->show_colors ? "│ " : " ", process->pid,
           separator);
    printf("%s%-*s%s%s", color(config, COLOR_WHITE), NAME_WIDTH, name, reset,
           separator);
    printf("%s%-7s%s%s", color(config, stateColor(process->state)), state,
           reset, separator);
    printf("%s%-*s%s%s", color(config, COLOR_CYAN), USER_WIDTH, user, reset,
           separator);
    printf("%s%12s%s%s\n", color(config, COLOR_MAGENTA), memory, reset,
           config->show_colors ? " │" : " ");
}

static void printProcesses(const MtopConfig *config, const SystemStats *stats)
{
    static const char *const plain_rule =
        "---------+--------------------+---------+--------------+--------------";

    if (config->show_colors) {
        fputs(COLOR_BLUE
              "┌─────────┬────────────────────┬─────────┬──────────────┬──────────────┐\n"
              "│   PID   │        NAME        │  STATE  │     USER     │    MEMORY    │\n"
              "├─────────┼────────────────────┼─────────┼──────────────┼──────────────┤"
              COLOR_RESET "\n",
              stdout);
    } else {
        printf("%s\n"
               "   PID   |        NAME        |  STATE  |     USER     |    MEMORY    \n"
               "%s\n",
               plain_rule, plain_rule);
    }

    for (size_t i = 0; i < stats->shown_count; i++) {
        printProcess(config, &stats->processes[i]);
    }

    if (config->show_colors) {
        fputs(COLOR_BLUE
              "└─────────┴────────────────────┴─────────┴──────────────┴──────────────┘"
              COLOR_RESET "\n",
              stdout);
    } else {
        puts(plain_rule);
    }
}

void displayBegin(const MtopConfig *config)
{
    if (config->show_colors) {
        fputs("\033[?25l", stdout);
    }
}

void displayEnd(const MtopConfig *config)
{
    printf("\n%sGoodbye!%s\n", color(config, COLOR_GREEN),
           color(config, COLOR_RESET));
    if (config->show_colors) {
        fputs("\033[?25h" COLOR_RESET, stdout);
    }
    fflush(stdout);
}

void displayStartMessage(const MtopConfig *config)
{
    printf("%sStarting mtop... Press 'h' for help or 'q' to quit%s\n",
           color(config, COLOR_GREEN), color(config, COLOR_RESET));
    fflush(stdout);
}

void displayFrame(const MtopConfig *config, const SystemStats *stats)
{
    clearScreen(config);
    printHeader(config);
    printSystemStats(config, stats);
    printProcesses(config, stats);
    printf("\n%s[q]uit [m]emory [c]pu [p]id [n]ame [r]everse [+/-] delay "
           "[h]elp | Update: %ds%s",
           color(config, COLOR_GRAY), config->update_interval,
           color(config, COLOR_RESET));
    fflush(stdout);
}

void displayHelp(const MtopConfig *config)
{
    clearScreen(config);
    printHeader(config);
    fputs("\nKeyboard Commands:\n"
          "  q, Q, ESC  - Quit\n"
          "  m, M       - Sort by Memory (default)\n"
          "  c, C       - Sort by CPU\n"
          "  p, P       - Sort by PID\n"
          "  n, N       - Sort by Name\n"
          "  r, R       - Reverse sort order\n"
          "  t, T       - Toggle network statistics\n"
          "  +, =       - Decrease update interval\n"
          "  -, _       - Increase update interval\n"
          "  h, H, ?    - Show this help\n\n"
          "Press any key to continue...",
          stdout);
    fflush(stdout);
}
