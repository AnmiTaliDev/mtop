#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "display.h"
#include "input.h"
#include "parser.h"
#include "system_info.h"
#include "terminal.h"
#include "view.h"

#define KEY_TIMEOUT_MS 100
#define FIRST_UPDATE_SECONDS 1

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t resized = 0;

static void handleSignal(int signal_number)
{
    (void)signal_number;
    running = 0;
}

static void handleResize(int signal_number)
{
    (void)signal_number;
    resized = 1;
}

static void installSignalHandlers(void)
{
    struct sigaction action = {.sa_handler = handleSignal};
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    action.sa_handler = handleResize;
    sigaction(SIGWINCH, &action, NULL);
}

static double secondsSince(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)(now.tv_sec - start->tv_sec) +
           (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

static bool runMonitor(MtopConfig *config, Terminal *terminal,
                       Display *display, SystemInfo *info, SystemStats *stats,
                       View *view)
{
    bool redraw = true;
    bool first_sample = true;
    struct timespec last_sample;
    clock_gettime(CLOCK_MONOTONIC, &last_sample);

    while (running) {
        if (resized) {
            resized = 0;
            redraw = true;
        }

        int interval = first_sample ? FIRST_UPDATE_SECONDS
                                    : config->update_interval;
        if (secondsSince(&last_sample) >= interval) {
            clock_gettime(CLOCK_MONOTONIC, &last_sample);
            if (!systemInfoSample(info, stats, config) ||
                !viewRefresh(view, stats)) {
                return false;
            }
            first_sample = false;
            redraw = true;
        }
        if (redraw) {
            if (!displayDraw(display, config, stats, view)) {
                return false;
            }
            redraw = false;
        }

        int key = terminalReadKey(terminal, KEY_TIMEOUT_MS);
        if (key == KEY_NONE) {
            continue;
        }

        InputResult result = inputHandle(key, config, stats, view);
        if (result == INPUT_QUIT) {
            break;
        }
        if (result == INPUT_FAILED) {
            return false;
        }
        redraw = result == INPUT_REDRAW;
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
    View view;
    viewInit(&view);
    bool success = systemInfoSample(&info, &stats, &config) &&
                   viewRefresh(&view, &stats);

    if (success) {
        Terminal terminal;
        terminalInit(&terminal);
        Display display;
        displayBegin(&display);

        success = runMonitor(&config, &terminal, &display, &info, &stats,
                             &view);

        displayEnd(&display);
        terminalRestore(&terminal);
    }

    if (!success) {
        fputs("Error: out of memory\n", stderr);
    }

    viewFree(&view);
    systemInfoFree(&info, &stats);
    configFree(&config);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
