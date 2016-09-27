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

#include <hpconio.h>
#include "display.h"


void *
get_pixel_font(const uint8_t **bytes, struct font *f)
{
	int page = 0[*bytes] - 0xA0;  // 区码
	int id   = 1[*bytes] - 0xA0;  // 位码
	if (page < 0) {               // ISO/IEC 2022 G0区
	    page = 3;                 // 用 GB 2312-1980 第3区中
	    id = 0[*bytes] - ' ';     // 相应的全角字符代替半角字符
	    *bytes += 1;              // 半角字符，指针移动一字节
	} else {                      // ISO/IEC 2022 GR区
	    *bytes += 2;              // 全角字符，指针移动两字节
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
}


const char *
bitmap_blit(const char *text, struct font *f)
{
	clear_screen();
	if (*text == '\n') {
		text++;  // omit line breaks between pages
	}
	int x = f->LEFT_MARGIN, y = f->TOP_MARGIN;

	// go through the text
	while (*text) {
		if (*text == '\n') {
			text++;
			x = SCREEN_WIDTH;
			y += f->ROWS;
			goto next;
		}

		// get the pixel font of current glyph
		uint8_t pos = 7, *ptr = get_pixel_font((const uint8_t **)&text, f);

		// draw the glyph
		for (size_t row = 0; row < f->ROWS; row++, y++) {
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
	set_indicator(INDICATOR_RSHIFT, *text);
	return text;
}
