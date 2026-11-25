#ifndef NETWORK_H
#define NETWORK_H

#include <net/if.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    bool present;
    bool has_rate;
    char name[IF_NAMESIZE];
    double rx_rate;
    double tx_rate;
} NetworkStats;

typedef struct {
    char name[IF_NAMESIZE];
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    struct timespec time;
} NetworkSample;

void networkUpdate(NetworkSample *previous, NetworkStats *stats);

#endif
