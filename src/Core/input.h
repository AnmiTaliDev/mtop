#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

#include "parser.h"
#include "system_info.h"
#include "view.h"

typedef enum {
    INPUT_IGNORED,
    INPUT_REDRAW,
    INPUT_QUIT,
    INPUT_FAILED
} InputResult;

InputResult inputHandle(int key, MtopConfig *config, SystemStats *stats,
                        View *view);

#endif
