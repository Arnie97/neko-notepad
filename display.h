/* Display manipulating module

Copyright (C) 2016 Arnie97

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#ifndef _DISPLAY_H
#define _DISPLAY_H

struct rom {
	int magic;
	struct font **fonts;
	int screen_height;
	const char *anti_piracy;
	const char *serial_no;
};

#define ROM             ((struct rom *)0x3FF00)
#define SCREEN_WIDTH    131
#define SCREEN_HEIGHT   (ROM->screen_height)
#define BYTES_PER_ROW   20

struct font {
	char ROWS;
	char COLS_STORAGE;
	char COLS_REAL;

	char LEFT_MARGIN;
	char TOP_MARGIN;
	char LINE_SPACING;

	int (*chunks)[2];
};

#define BYTES_PER_GLYPH(f) ((f->ROWS * f->COLS_STORAGE + 7) / 8)

#include <stdint.h>
extern uint8_t *__display_buf;

#define indicator(n) __display_buf[BYTES_PER_ROW * (n) + (SCREEN_WIDTH >> 3)]
#define INDICATOR_MASK (1 << (SCREEN_WIDTH & 7))
#define get_indicator(n) (indicator(n) | INDICATOR_MASK)
#define set_indicator(n, value) { \
	if (value) indicator(n) |= INDICATOR_MASK; \
	else indicator(n) &= ~INDICATOR_MASK; \
};

#define INDICATOR_REMOTE    0
#define INDICATOR_LSHIFT    1
#define INDICATOR_RSHIFT    2
#define INDICATOR_ALPHA     3
#define INDICATOR_BATTERY   4
#define INDICATOR_WAIT      5

const char *bitmap_blit(const char *text, struct font *f);

#endif
