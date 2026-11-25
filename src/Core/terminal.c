#include "terminal.h"

#include <poll.h>
#include <string.h>
#include <unistd.h>

#define ESCAPE_TIMEOUT_MS 50
#define SEQUENCE_SIZE 16

static bool waitForInput(int timeout_ms)
{
    struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN};

    return poll(&input, 1, timeout_ms) > 0;
}

static bool readByte(unsigned char *byte, int timeout_ms)
{
    return waitForInput(timeout_ms) && read(STDIN_FILENO, byte, 1) == 1;
}

static int decodeSequence(const char *sequence)
{
    static const struct {
        const char *sequence;
        int key;
    } keys[] = {
        {"A", KEY_UP},         {"B", KEY_DOWN},       {"H", KEY_HOME},
        {"F", KEY_END},        {"1~", KEY_HOME},      {"4~", KEY_END},
        {"5~", KEY_PAGE_UP},   {"6~", KEY_PAGE_DOWN}, {"7~", KEY_HOME},
        {"8~", KEY_END},
    };

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        if (strcmp(sequence, keys[i].sequence) == 0) {
            return keys[i].key;
        }
    }

    return KEY_SEQUENCE;
}

static int readEscape(void)
{
    unsigned char byte;
    if (!readByte(&byte, ESCAPE_TIMEOUT_MS)) {
        return KEY_ESCAPE;
    }
    if (byte != '[' && byte != 'O') {
        return KEY_SEQUENCE;
    }

    char sequence[SEQUENCE_SIZE] = "";
    size_t length = 0;
    while (length + 1 < sizeof(sequence) &&
           readByte(&byte, ESCAPE_TIMEOUT_MS)) {
        sequence[length++] = (char)byte;
        if (byte >= 0x40 && byte <= 0x7E) {
            break;
        }
    }

    return decodeSequence(sequence);
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

    return key == KEY_ESCAPE ? readEscape() : key;
}
