#include "parser.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#define SYSTEM_CONFIG_PATH "/etc/mtop/config"

#define MIN_UPDATE_INTERVAL 1
#define MAX_UPDATE_INTERVAL 60

static char *trim(char *str)
{
    while (isspace((unsigned char)*str)) {
        str++;
    }

    char *end = str + strlen(str);
    while (end > str && isspace((unsigned char)end[-1])) {
        end--;
    }
    *end = '\0';

    return str;
}

static void stringListFree(StringList *list)
{
    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
}

static bool stringListAppend(StringList *list, const char *item)
{
    char **items = realloc(list->items, (list->count + 1) * sizeof(*items));
    if (!items) {
        return false;
    }
    list->items = items;

    char *copy = strdup(item);
    if (!copy) {
        return false;
    }
    list->items[list->count++] = copy;

    return true;
}

static bool stringListParse(StringList *list, char *value)
{
    stringListFree(list);

    char *state = NULL;
    for (char *token = strtok_r(value, ",", &state); token;
         token = strtok_r(NULL, ",", &state)) {
        const char *item = trim(token);
        if (*item && !stringListAppend(list, item)) {
            return false;
        }
    }

    return true;
}

static bool parseBool(const char *value)
{
    return strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 ||
           strcasecmp(value, "on") == 0 || strcmp(value, "1") == 0;
}

static bool parseClampedInt(const char *value, int min, int max, int *result)
{
    char *end = NULL;
    errno = 0;
    long number = strtol(value, &end, 10);
    if (end == value || *end != '\0' || errno == ERANGE) {
        return false;
    }

    if (number < min) {
        number = min;
    } else if (number > max) {
        number = max;
    }
    *result = (int)number;

    return true;
}

static SortBy parseSortBy(const char *value)
{
    if (strcasecmp(value, "cpu") == 0) {
        return SORT_CPU;
    }
    if (strcasecmp(value, "pid") == 0) {
        return SORT_PID;
    }
    if (strcasecmp(value, "name") == 0) {
        return SORT_NAME;
    }

    return SORT_MEMORY;
}

static bool parseTheme(const char *value, ThemeId *theme)
{
    static const char *const names[THEME_COUNT] = {
        [THEME_DEFAULT] = "default",
        [THEME_OCEAN] = "ocean",
        [THEME_AMBER] = "amber",
        [THEME_MONO] = "mono",
    };

    for (int i = 0; i < THEME_COUNT; i++) {
        if (strcasecmp(value, names[i]) == 0) {
            *theme = (ThemeId)i;
            return true;
        }
    }

    return false;
}

static bool parseEntry(MtopConfig *config, const char *key, char *value)
{
    if (strcmp(key, "update_interval") == 0) {
        return parseClampedInt(value, MIN_UPDATE_INTERVAL, MAX_UPDATE_INTERVAL,
                               &config->update_interval);
    }
    if (strcmp(key, "theme") == 0) {
        return parseTheme(value, &config->theme);
    }
    if (strcmp(key, "hide_processes") == 0) {
        return stringListParse(&config->hide_processes, value);
    }
    if (strcmp(key, "show_only_users") == 0) {
        return stringListParse(&config->show_only_users, value);
    }

    if (strcmp(key, "show_colors") == 0) {
        config->show_colors = parseBool(value);
    } else if (strcmp(key, "show_load_avg") == 0) {
        config->show_load_avg = parseBool(value);
    } else if (strcmp(key, "show_memory_bar") == 0) {
        config->show_memory_bar = parseBool(value);
    } else if (strcmp(key, "show_cpu_bar") == 0) {
        config->show_cpu_bar = parseBool(value);
    } else if (strcmp(key, "show_network_stats") == 0) {
        config->show_network_stats = parseBool(value);
    } else if (strcmp(key, "sort_by") == 0) {
        config->sort_by = parseSortBy(value);
    } else if (strcmp(key, "reverse_sort") == 0) {
        config->reverse_sort = parseBool(value);
    } else if (strcmp(key, "show_process_state") == 0) {
        config->show_process_state = parseBool(value);
    } else if (strcmp(key, "show_process_user") == 0) {
        config->show_process_user = parseBool(value);
    } else if (strcmp(key, "show_kernel_threads") == 0) {
        config->show_kernel_threads = parseBool(value);
    } else if (strcmp(key, "max_processes") != 0 &&
               strcmp(key, "progress_bar_width") != 0) {
        return false;
    }

    return true;
}

static bool parseLine(MtopConfig *config, char *line)
{
    char *text = trim(line);
    if (*text == '\0' || *text == '#' || *text == '[') {
        return true;
    }

    char *separator = strchr(text, '=');
    if (!separator) {
        return false;
    }
    *separator = '\0';

    return parseEntry(config, trim(text), trim(separator + 1));
}

void configInit(MtopConfig *config)
{
    *config = (MtopConfig){
        .update_interval = 2,
        .show_colors = true,
        .show_load_avg = true,
        .show_memory_bar = true,
        .show_cpu_bar = true,
        .show_network_stats = true,
        .sort_by = SORT_MEMORY,
        .show_process_state = true,
        .show_process_user = true,
    };
}

void configFree(MtopConfig *config)
{
    stringListFree(&config->hide_processes);
    stringListFree(&config->show_only_users);
}

bool configLoad(MtopConfig *config, const char *path)
{
    FILE *file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error: cannot open config file %s: %s\n", path,
                strerror(errno));
        return false;
    }

    char *line = NULL;
    size_t capacity = 0;
    for (size_t number = 1; getline(&line, &capacity, file) != -1; number++) {
        if (!parseLine(config, line)) {
            fprintf(stderr, "Warning: invalid config line %s:%zu\n", path,
                    number);
        }
    }

    bool success = feof(file) != 0;
    if (!success) {
        fprintf(stderr, "Error: cannot read config file %s: %s\n", path,
                strerror(errno));
    }

    free(line);
    fclose(file);

    return success;
}

static const char *homeDirectory(void)
{
    const char *home = getenv("HOME");
    if (home) {
        return home;
    }

    const struct passwd *entry = getpwuid(getuid());

    return entry ? entry->pw_dir : NULL;
}

void configLoadDefault(MtopConfig *config)
{
    const char *home = homeDirectory();
    if (home) {
        char path[PATH_MAX];
        int length = snprintf(path, sizeof(path), "%s/.config/mtop/config",
                              home);
        if (length > 0 && (size_t)length < sizeof(path) &&
            access(path, F_OK) == 0) {
            configLoad(config, path);
            return;
        }
    }

    if (access(SYSTEM_CONFIG_PATH, F_OK) == 0) {
        configLoad(config, SYSTEM_CONFIG_PATH);
    }
}

static void printHelp(void)
{
    puts("mtop - Minimalistic Modern Top\n"
         "\n"
         "Usage: mtop [OPTIONS]\n"
         "\n"
         "Options:\n"
         "  -h, --help              Show this help message\n"
         "  -c, --config FILE       Use specified configuration file\n"
         "  -d, --delay SECONDS     Update interval in seconds (1-60)\n"
         "  -n, --max-processes N   Ignored, the process list scrolls\n"
         "  --no-color              Disable colored output\n"
         "  --sort-memory           Sort processes by memory usage (default)\n"
         "  --sort-cpu              Sort processes by CPU usage\n"
         "  --sort-pid              Sort processes by PID\n"
         "  --sort-name             Sort processes by name\n"
         "  --reverse               Reverse sort order\n"
         "\n"
         "Configuration files:\n"
         "  ~/.config/mtop/config   User configuration\n"
         "  " SYSTEM_CONFIG_PATH "        System configuration");
}

static bool matches(const char *arg, const char *short_name,
                    const char *long_name)
{
    return strcmp(arg, short_name) == 0 || strcmp(arg, long_name) == 0;
}

ArgsResult configParseArgs(MtopConfig *config, int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (matches(arg, "-h", "--help")) {
            printHelp();
            return ARGS_EXIT;
        }

        if (matches(arg, "-c", "--config")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires a file path\n", arg);
                return ARGS_ERROR;
            }
            if (!configLoad(config, argv[++i])) {
                return ARGS_ERROR;
            }
        } else if (matches(arg, "-d", "--delay")) {
            if (i + 1 >= argc ||
                !parseClampedInt(argv[++i], MIN_UPDATE_INTERVAL,
                                 MAX_UPDATE_INTERVAL,
                                 &config->update_interval)) {
                fprintf(stderr, "Error: %s requires a number\n", arg);
                return ARGS_ERROR;
            }
        } else if (matches(arg, "-n", "--max-processes")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: %s requires a number\n", arg);
                return ARGS_ERROR;
            }
            i++;
        } else if (strcmp(arg, "--no-color") == 0) {
            config->show_colors = false;
        } else if (strcmp(arg, "--sort-memory") == 0) {
            config->sort_by = SORT_MEMORY;
        } else if (strcmp(arg, "--sort-cpu") == 0) {
            config->sort_by = SORT_CPU;
        } else if (strcmp(arg, "--sort-pid") == 0) {
            config->sort_by = SORT_PID;
        } else if (strcmp(arg, "--sort-name") == 0) {
            config->sort_by = SORT_NAME;
        } else if (strcmp(arg, "--reverse") == 0) {
            config->reverse_sort = true;
        } else {
            fprintf(stderr, "Warning: unknown argument: %s\n", arg);
        }
    }

    return ARGS_RUN;
}
