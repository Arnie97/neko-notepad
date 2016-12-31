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
#include <hpstring.h>
#include <saturn.h>
#include "s3c2410.h"
#include "display.h"

static uint8_t frame[SCREEN_HEIGHT][WIDTH_IN_BYTES];
static int32_t interval;


void
draw(uint8_t *frame)
{
	for (int row = 0; row < SCREEN_HEIGHT; row++) {
		memcpy(
			&__display_buf[BYTES_PER_ROW * row + LEFT_MARGIN],
			&frame[WIDTH_IN_BYTES * row],
			WIDTH_IN_BYTES
		);
	}
	delay(interval);
}


int
main(void)
{
	if (ROM->magic != 0xBadA991e) {
		return 1;
	} else {
		interval = sat_pop_zint_llong();
	}

	unsigned row = -1;
	for (uint8_t *p = ROM->beg; p != ROM->end; p++) {
		if (on_pressed) {
			return 0;
		}
		if (*p & 0x80) {
			row++;
			if (row == 64) {
				row = 0;
				draw((uint8_t *)frame);
			}
		}
		if (*p != 0xFF) {
			uint8_t byte = *p & 0x7F;
			frame[row][byte] = *++p;
		}
	}
	while (!on_pressed);
}
