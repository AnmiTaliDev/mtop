#ifndef CONTROL_H
#define CONTROL_H

#include <stddef.h>
#include <stdint.h>

void controlTerminate(int pid, uint64_t start_time, char *message,
                      size_t size);

#endif
