#include "theme.h"

static const char *const themes[THEME_COUNT][STYLE_COUNT] = {
    [THEME_DEFAULT] = {
        [STYLE_DEFAULT] = "0",
        [STYLE_BORDER] = "0;90",
        [STYLE_TITLE] = "0;1;36",
        [STYLE_MUTED] = "0;90",
        [STYLE_HEADER] = "0;1",
        [STYLE_ACCENT] = "0;36",
        [STYLE_LOW] = "0;32",
        [STYLE_MEDIUM] = "0;33",
        [STYLE_HIGH] = "0;31",
        [STYLE_SELECTED] = "0;7",
    },
    [THEME_OCEAN] = {
        [STYLE_DEFAULT] = "0",
        [STYLE_BORDER] = "0;34",
        [STYLE_TITLE] = "0;1;94",
        [STYLE_MUTED] = "0;90",
        [STYLE_HEADER] = "0;1;96",
        [STYLE_ACCENT] = "0;94",
        [STYLE_LOW] = "0;36",
        [STYLE_MEDIUM] = "0;94",
        [STYLE_HIGH] = "0;95",
        [STYLE_SELECTED] = "0;30;46",
    },
    [THEME_AMBER] = {
        [STYLE_DEFAULT] = "0;33",
        [STYLE_BORDER] = "0;2;33",
        [STYLE_TITLE] = "0;1;93",
        [STYLE_MUTED] = "0;2;33",
        [STYLE_HEADER] = "0;1;33",
        [STYLE_ACCENT] = "0;33",
        [STYLE_LOW] = "0;33",
        [STYLE_MEDIUM] = "0;93",
        [STYLE_HIGH] = "0;1;31",
        [STYLE_SELECTED] = "0;30;43",
    },
    [THEME_MONO] = {
        [STYLE_DEFAULT] = "0",
        [STYLE_BORDER] = "0;2",
        [STYLE_TITLE] = "0;1",
        [STYLE_MUTED] = "0;2",
        [STYLE_HEADER] = "0;1",
        [STYLE_ACCENT] = "0",
        [STYLE_LOW] = "0",
        [STYLE_MEDIUM] = "0;1",
        [STYLE_HIGH] = "0;1;4",
        [STYLE_SELECTED] = "0;7",
    },
};

const char *const *themeStyles(ThemeId theme)
{
    return themes[theme];
}
