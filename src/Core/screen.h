#ifndef SCREEN_H
#define SCREEN_H

#include <stdbool.h>

#define GLYPH_SIZE 5

typedef enum {
    STYLE_DEFAULT,
    STYLE_BORDER,
    STYLE_TITLE,
    STYLE_MUTED,
    STYLE_HEADER,
    STYLE_ACCENT,
    STYLE_LOW,
    STYLE_MEDIUM,
    STYLE_HIGH,
    STYLE_SELECTED,
    STYLE_COUNT
} Style;

typedef struct {
    char glyph[GLYPH_SIZE];
    Style style;
} Cell;

typedef struct {
    Cell *cells;
    int width;
    int height;
} Screen;

bool screenReset(Screen *screen, int width, int height);
void screenFree(Screen *screen);
void screenPut(Screen *screen, int x, int y, const char *glyph, Style style);
int screenText(Screen *screen, int x, int y, int max_width, const char *text,
               Style style);
void screenBox(Screen *screen, int x, int y, int width, int height,
               bool ascii);
void screenFlush(const Screen *screen, const char *const *styles);

#endif
