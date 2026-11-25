#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "display.h"
#include "parser.h"
#include "system_info.h"
#include "terminal.h"

#define KEY_TIMEOUT_MS 100
#define MAX_KEY_INTERVAL 10

typedef enum {
    ACTION_NONE,
    ACTION_REDRAW,
    ACTION_HELP,
    ACTION_QUIT
} Action;

static volatile sig_atomic_t running = 1;

static void handleSignal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static void installSignalHandlers(void)
{
    struct sigaction action = {.sa_handler = handleSignal};
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

static Action setSortOrder(MtopConfig *config, SortBy sort_by)
{
    config->sort_by = sort_by;
    return ACTION_REDRAW;
}

static Action handleKey(MtopConfig *config, int key)
{
    switch (key) {
    case 'q':
    case 'Q':
    case KEY_ESCAPE:
        return ACTION_QUIT;
    case 'm':
    case 'M':
        return setSortOrder(config, SORT_MEMORY);
    case 'c':
    case 'C':
        return setSortOrder(config, SORT_CPU);
    case 'p':
    case 'P':
        return setSortOrder(config, SORT_PID);
    case 'n':
    case 'N':
        return setSortOrder(config, SORT_NAME);
    case 'r':
    case 'R':
        config->reverse_sort = !config->reverse_sort;
        return ACTION_REDRAW;
    case 't':
    case 'T':
        config->show_network_stats = !config->show_network_stats;
        return ACTION_REDRAW;
    case '+':
    case '=':
        if (config->update_interval <= 1) {
            return ACTION_NONE;
        }
        config->update_interval--;
        return ACTION_REDRAW;
    case '-':
    case '_':
        if (config->update_interval >= MAX_KEY_INTERVAL) {
            return ACTION_NONE;
        }
        config->update_interval++;
        return ACTION_REDRAW;
    case 'h':
    case 'H':
    case '?':
        return ACTION_HELP;
    default:
        return ACTION_NONE;
    }
}

static double secondsSince(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start->tv_sec) +
           (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

static void showHelp(const MtopConfig *config, Terminal *terminal)
{
    displayHelp(config);
    while (running && terminalReadKey(terminal, KEY_TIMEOUT_MS) == KEY_NONE) {
    }
}

static bool runMonitor(MtopConfig *config, Terminal *terminal,
                       SystemInfo *info, SystemStats *stats)
{
    bool redraw = true;
    struct timespec last_update = {0};

    while (running) {
        Action action = handleKey(config,
                                  terminalReadKey(terminal, KEY_TIMEOUT_MS));
        if (action == ACTION_QUIT) {
            break;
        }
        if (action == ACTION_HELP) {
            showHelp(config, terminal);
        }
        if (action != ACTION_NONE) {
            redraw = true;
        }

        if (running &&
            (redraw ||
             secondsSince(&last_update) >= config->update_interval)) {
            clock_gettime(CLOCK_MONOTONIC, &last_update);
            if (!systemInfoUpdate(info, stats, config)) {
                return false;
            }
            displayFrame(config, stats);
            redraw = false;
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    MtopConfig config;
    configInit(&config);
    configLoadDefault(&config);

    ArgsResult args = configParseArgs(&config, argc, argv);
    if (args != ARGS_RUN) {
        configFree(&config);
        return args == ARGS_EXIT ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    installSignalHandlers();

    SystemInfo info;
    SystemStats stats;
    systemInfoInit(&info, &stats);
    bool success = systemInfoUpdate(&info, &stats, &config);

    if (success) {
        Terminal terminal;
        terminalInit(&terminal);
        displayBegin(&config);
        displayStartMessage(&config);

        const struct timespec start_delay = {.tv_sec = 1};
        nanosleep(&start_delay, NULL);

        success = runMonitor(&config, &terminal, &info, &stats);

        displayEnd(&config);
        terminalRestore(&terminal);
    }

    if (!success) {
        fputs("Error: out of memory\n", stderr);
    }

    systemInfoFree(&info, &stats);
    configFree(&config);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
