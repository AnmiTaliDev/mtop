#ifndef DISPLAY_H
#define DISPLAY_H

#include "parser.h"
#include "system_info.h"

void displayBegin(const MtopConfig *config);
void displayEnd(const MtopConfig *config);
void displayStartMessage(const MtopConfig *config);
void displayFrame(const MtopConfig *config, const SystemStats *stats);
void displayHelp(const MtopConfig *config);

#endif
