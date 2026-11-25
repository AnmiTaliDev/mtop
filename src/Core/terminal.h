#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <termios.h>

#define KEY_NONE 0
#define KEY_ESCAPE 27
#define KEY_SEQUENCE (-1)
#define KEY_UP (-2)
#define KEY_DOWN (-3)
#define KEY_PAGE_UP (-4)
#define KEY_PAGE_DOWN (-5)
#define KEY_HOME (-6)
#define KEY_END (-7)

typedef struct {
    struct termios original;
    bool active;
} Terminal;

void terminalInit(Terminal *terminal);
void terminalRestore(const Terminal *terminal);
int terminalReadKey(Terminal *terminal, int timeout_ms);

#endif
