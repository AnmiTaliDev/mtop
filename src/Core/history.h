#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

#define HISTORY_SIZE 512

typedef struct {
    double values[HISTORY_SIZE];
    size_t next;
    size_t count;
} History;

void historyPush(History *history, double value);
double historyGet(const History *history, size_t age);

#endif
