#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>

#include "parser.h"
#include "screen.h"
#include "system_info.h"

typedef struct {
    Screen screen;
} Display;

void displayBegin(Display *display);
void displayEnd(Display *display);
bool displayDraw(Display *display, const MtopConfig *config,
                 const SystemStats *stats, bool show_help);

#endif
