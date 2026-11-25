#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <termios.h>

#define KEY_NONE 0
#define KEY_SEQUENCE (-1)
#define KEY_ESCAPE 27

typedef struct {
    struct termios original;
    bool active;
} Terminal;

void terminalInit(Terminal *terminal);
void terminalRestore(const Terminal *terminal);
int terminalReadKey(Terminal *terminal, int timeout_ms);

#endif
