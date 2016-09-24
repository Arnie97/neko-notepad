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

#include <stdint.h>
#include <syscall.h>
#include <hpstring.h>
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


int
font_not_found(void)
{
	if (ROM->magic == 0xC0DEBA5E) {
		return 0;
	}
	const uint8_t *msg = (
		"\x08\xa1\x40\x01\x82\x12\x08\x00\x20\x80\x00"
		"\xd0\xa7\xcf\xcf\x3f\x7f\x08\x10\xfc\xf3\x87"
		"\x00\x51\x2a\x41\xa0\x12\x7f\xfe\x04\x92\x80"
		"\xd8\x27\x07\x01\x82\x7f\x49\x44\xf8\xf0\x87"
		"\x90\xa2\xea\xdf\x3f\x04\x49\x28\x40\x50\x81"
		"\x90\x43\x80\x02\x09\x2b\x7f\x10\xfc\xf3\x87"
		"\xb0\xa2\x4a\x12\x86\x12\x08\x28\x20\x10\x01"
		"\x90\xd2\x2b\x9e\x19\x66\x08\xc6\x30\x08\x81"
	);
	for (int row = 0; row < 8; row++) {
		memcpy(&__display_buf[(row + 18) * BYTES_PER_ROW], &msg[row * 11], 11);
	}
	return 1;
}


const char *
bitmap_blit(const char *text, struct font *f)
{
	SysCall(ClearLcdEntry);
	if (font_not_found()) {
		return text;
	} else if (*text == '\n') {
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
