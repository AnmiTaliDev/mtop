#include "screen.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const style_codes[STYLE_COUNT] = {
    [STYLE_DEFAULT] = "0",
    [STYLE_BORDER] = "0;90",
    [STYLE_TITLE] = "0;1;36",
    [STYLE_MUTED] = "0;90",
    [STYLE_HEADER] = "0;1",
    [STYLE_ACCENT] = "0;36",
    [STYLE_LOW] = "0;32",
    [STYLE_MEDIUM] = "0;33",
    [STYLE_HIGH] = "0;31",
};

bool screenReset(Screen *screen, int width, int height)
{
    if (width != screen->width || height != screen->height) {
        Cell *cells = realloc(screen->cells,
                              (size_t)width * (size_t)height * sizeof(*cells));
        if (!cells) {
            return false;
        }
        screen->cells = cells;
        screen->width = width;
        screen->height = height;
    }

    for (int i = 0; i < width * height; i++) {
        screen->cells[i] = (Cell){.glyph = " ", .style = STYLE_DEFAULT};
    }

    return true;
}

void screenFree(Screen *screen)
{
    free(screen->cells);
    *screen = (Screen){0};
}

void screenPut(Screen *screen, int x, int y, const char *glyph, Style style)
{
    if (x < 0 || y < 0 || x >= screen->width || y >= screen->height) {
        return;
    }

    Cell *cell = &screen->cells[y * screen->width + x];
    snprintf(cell->glyph, sizeof(cell->glyph), "%s", glyph);
    cell->style = style;
}

static int sequenceLength(const unsigned char *text)
{
    int length = 0;
    if (text[0] < 0x80) {
        length = 1;
    } else if (text[0] >= 0xC2 && text[0] <= 0xDF) {
        length = 2;
    } else if (text[0] >= 0xE0 && text[0] <= 0xEF) {
        length = 3;
    } else if (text[0] >= 0xF0 && text[0] <= 0xF4) {
        length = 4;
    }

    for (int i = 1; i < length; i++) {
        if ((text[i] & 0xC0) != 0x80) {
            return 0;
        }
    }

    return length;
}

static bool isControl(const unsigned char *text, int length)
{
    if (length == 1) {
        return text[0] < 0x20 || text[0] == 0x7F;
    }

    return length == 2 && text[0] == 0xC2 && text[1] < 0xA0;
}

int screenText(Screen *screen, int x, int y, int max_width, const char *text,
               Style style)
{
    const unsigned char *next = (const unsigned char *)text;
    int column = 0;

    while (*next && column < max_width) {
        int length = sequenceLength(next);
        char glyph[GLYPH_SIZE] = "?";
        if (length > 0 && !isControl(next, length)) {
            memcpy(glyph, next, (size_t)length);
            glyph[length] = '\0';
        }
        screenPut(screen, x + column, y, glyph, style);
        next += length > 0 ? length : 1;
        column++;
    }

    return column;
}

void screenBox(Screen *screen, int x, int y, int width, int height, bool ascii)
{
    int right = x + width - 1;
    int bottom = y + height - 1;

    for (int column = x + 1; column < right; column++) {
        screenPut(screen, column, y, ascii ? "-" : "─", STYLE_BORDER);
        screenPut(screen, column, bottom, ascii ? "-" : "─", STYLE_BORDER);
    }
    for (int row = y + 1; row < bottom; row++) {
        screenPut(screen, x, row, ascii ? "|" : "│", STYLE_BORDER);
        screenPut(screen, right, row, ascii ? "|" : "│", STYLE_BORDER);
    }

    screenPut(screen, x, y, ascii ? "+" : "╭", STYLE_BORDER);
    screenPut(screen, right, y, ascii ? "+" : "╮", STYLE_BORDER);
    screenPut(screen, x, bottom, ascii ? "+" : "╰", STYLE_BORDER);
    screenPut(screen, right, bottom, ascii ? "+" : "╯", STYLE_BORDER);
}

void screenFlush(const Screen *screen, bool colors)
{
    fputs("\033[H", stdout);

    for (int y = 0; y < screen->height; y++) {
        Style current = STYLE_COUNT;
        for (int x = 0; x < screen->width; x++) {
            const Cell *cell = &screen->cells[y * screen->width + x];
            if (colors && cell->style != current) {
                current = cell->style;
                printf("\033[%sm", style_codes[current]);
            }
            fputs(cell->glyph, stdout);
        }
        if (colors) {
            fputs("\033[0m", stdout);
        }
        if (y + 1 < screen->height) {
            fputs("\r\n", stdout);
        }
    }

    fflush(stdout);
}
