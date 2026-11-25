#include "history.h"

void historyPush(History *history, double value)
{
    history->values[history->next] = value;
    history->next = (history->next + 1) % HISTORY_SIZE;
    if (history->count < HISTORY_SIZE) {
        history->count++;
    }
}

double historyGet(const History *history, size_t age)
{
    size_t index = (history->next + HISTORY_SIZE - 1 - age) % HISTORY_SIZE;

    return history->values[index];
}
