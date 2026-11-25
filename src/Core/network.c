#include "network.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static void readDefaultInterface(char *name, size_t size)
{
    name[0] = '\0';

    FILE *file = fopen("/proc/net/route", "r");
    if (!file) {
        return;
    }

    unsigned long best_metric = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char interface[IF_NAMESIZE];
        unsigned long destination, metric, mask;
        if (sscanf(line, "%15s %lx %*x %*x %*u %*u %lu %lx", interface,
                   &destination, &metric, &mask) != 4) {
            continue;
        }
        if (destination == 0 && mask == 0 &&
            (name[0] == '\0' || metric < best_metric)) {
            snprintf(name, size, "%s", interface);
            best_metric = metric;
        }
    }
    fclose(file);
}

static bool parseInterfaceLine(char *line, NetworkSample *sample)
{
    char *separator = strchr(line, ':');
    if (!separator) {
        return false;
    }
    *separator = '\0';

    const char *name = line;
    while (isspace((unsigned char)*name)) {
        name++;
    }
    if (*name == '\0' || strcmp(name, "lo") == 0) {
        return false;
    }
    snprintf(sample->name, sizeof(sample->name), "%s", name);

    return sscanf(separator + 1,
                  "%" SCNu64 " %*u %*u %*u %*u %*u %*u %*u %" SCNu64,
                  &sample->rx_bytes, &sample->tx_bytes) == 2;
}

static bool readInterface(NetworkSample *selected)
{
    FILE *file = fopen("/proc/net/dev", "r");
    if (!file) {
        return false;
    }

    char default_name[IF_NAMESIZE];
    readDefaultInterface(default_name, sizeof(default_name));

    bool found = false;
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        NetworkSample sample = {0};
        if (!parseInterfaceLine(line, &sample)) {
            continue;
        }
        if (strcmp(sample.name, default_name) == 0) {
            *selected = sample;
            found = true;
            break;
        }
        if (!found || sample.rx_bytes + sample.tx_bytes >
                          selected->rx_bytes + selected->tx_bytes) {
            *selected = sample;
            found = true;
        }
    }
    fclose(file);

    return found;
}

void networkUpdate(NetworkSample *previous, NetworkStats *stats)
{
    *stats = (NetworkStats){0};

    NetworkSample current = {0};
    if (!readInterface(&current)) {
        previous->name[0] = '\0';
        return;
    }
    clock_gettime(CLOCK_MONOTONIC, &current.time);

    stats->present = true;
    snprintf(stats->name, sizeof(stats->name), "%s", current.name);

    double elapsed = (double)(current.time.tv_sec - previous->time.tv_sec) +
                     (double)(current.time.tv_nsec - previous->time.tv_nsec) /
                         1e9;
    if (strcmp(current.name, previous->name) == 0 && elapsed > 0.0 &&
        current.rx_bytes >= previous->rx_bytes &&
        current.tx_bytes >= previous->tx_bytes) {
        stats->has_rate = true;
        stats->rx_rate = (double)(current.rx_bytes - previous->rx_bytes) /
                         elapsed;
        stats->tx_rate = (double)(current.tx_bytes - previous->tx_bytes) /
                         elapsed;
    }

    *previous = current;
}
