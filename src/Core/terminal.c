#include "terminal.h"

#include <poll.h>
#include <unistd.h>

#define ESCAPE_TIMEOUT_MS 50

static bool waitForInput(int timeout_ms)
{
    struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN};

    return poll(&input, 1, timeout_ms) > 0;
}

void terminalInit(Terminal *terminal)
{
    terminal->active = false;
    if (tcgetattr(STDIN_FILENO, &terminal->original) != 0) {
        return;
    }

    struct termios raw = terminal->original;
    raw.c_lflag &= ~(tcflag_t)(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    terminal->active = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
}

void terminalRestore(const Terminal *terminal)
{
    if (terminal->active) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal->original);
    }
}

int terminalReadKey(Terminal *terminal, int timeout_ms)
{
    if (!terminal->active) {
        poll(NULL, 0, timeout_ms);
        return KEY_NONE;
    }

    if (!waitForInput(timeout_ms)) {
        return KEY_NONE;
    }

    unsigned char key;
    if (read(STDIN_FILENO, &key, 1) != 1) {
        poll(NULL, 0, timeout_ms);
        return KEY_NONE;
    }

    if (key == KEY_ESCAPE && waitForInput(ESCAPE_TIMEOUT_MS)) {
        unsigned char rest[32];
        if (read(STDIN_FILENO, rest, sizeof(rest)) > 0) {
            return KEY_SEQUENCE;
        }
    }

    return key;
}
