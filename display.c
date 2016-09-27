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

#include "satstr.h"
#include <hpconio.h>
#include "stack.h"
#include "display.h"


void *
get_pixel_font(SAT_STRING *str, struct font *f)
{
	int page = peek(str) - 0xA0;  // 区码
	str->cursor++;
	int id   = peek(str) - 0xA0;  // 位码
	if (page < 0) {               // ISO/IEC 2022 G0区
		id = page + 0xA0 - ' ';   // 用相应的全角字符代替半角字符
		page = 3;                 // 这些全角字符在 GB 2312-1980 第3区
	} else {                      // ISO/IEC 2022 GR区
		str->cursor++;            // 全角字符，指针需移动两字节
	}

	size_t code_point = ((page - 1) * 94 + (id - 1));
	int (*offset)[2] = f->chunks;
	for (int i = 0; offset[i - 1][1]; i++) {
		if (code_point >= offset[i][1]) {
			code_point -= offset[i][1];
			code_point *= BYTES_PER_GLYPH(f);
			code_point += offset[i][0];
			return (void *)code_point;
		}
	}
	return NULL;
}


void
bitmap_blit(SAT_STRING *str, struct font *f)
{
	clear_screen();
	if (peek(str) == '\n') {
		str->cursor++;  // omit line breaks between pages
	}
	int x = f->LEFT_MARGIN, y = f->TOP_MARGIN;

	// go through the text
	while (str->cursor != str->end) {
		if (peek(str) == '\n') {
			str->cursor++;
			x = SCREEN_WIDTH;
			y += f->ROWS;
			goto next;
		}

		// get the pixel font of current glyph
		uint8_t *ptr = check_ptr(get_pixel_font(str, f));

		// draw the glyph
		for (size_t row = 0, pos = 7; row < f->ROWS; row++, y++) {
			for (size_t col = 0; col < f->COLS_STORAGE; col++, x++) {

				// fill the pixels bit by bit
				unsigned pixel = ((*ptr >> pos) & 1);
				__display_buf[y * BYTES_PER_ROW + (x >> 3)] |= pixel << (x & 7);

				// byte alignment
				if (pos) {
					pos--;
				} else {
					pos = 7;
					ptr++;
				}
			}
			x -= f->COLS_STORAGE;
		}

		next: if (x + f->COLS_REAL <= SCREEN_WIDTH - f->COLS_REAL) {  // next char
			x += f->COLS_REAL;
			y -= f->ROWS;
		} else if (y + f->LINE_SPACING + f->ROWS <= SCREEN_HEIGHT) {  // next line
			x = f->LEFT_MARGIN;
			y += f->LINE_SPACING;
		} else {  // next page
			break;
		}
	}
	set_indicator(INDICATOR_RSHIFT, str->cursor != str->end);
}
