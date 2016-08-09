/* A simple user interface for this project

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
#include <hpsys.h>
#include <hpstdio.h>
#include <hpconio.h>
#include <hpgraphics.h>
#include "hp39kbd.h"
#include "main.h"


int
event_handler(unsigned row, unsigned col)
{
	// [APLET]
	if (row == 0 && col == 7) {
		// exit immediately
		return 25;
	} else {
		// wait until the key is released
		while (any_key_pressed);
	}

	// [UP]: 0, [LEFT]: 1, [DOWN]: 2, [RIGHT]: 3
	if (col == 6 && row < 4) {
		return row + 20;
	} else if (row <= 2 && col <= 4) {
		int ch = row * 5 - col + 0xD;  // letter keys
		if (ch == 0xD) {
			return -1;  // [DEL]
		} else if (ch < 0xD) {
			ch++;  // skip the [DEL] key after [D]
		} else if (ch > 0xF) {
			return -1;
		}
		return ch;
	} else if (row >= 3 && 1 <= col && col <= 3) {
		return row == 6? (
			col == 3? 0: -1
		):(
			(6 - row) * 3 - col + 1
		);
	}

	// unhandled keys
	return -1;
}


int
hex_viewer(void *address, int cursor)
{
	clear_screen();
	// print first 2 bits in the memory address
	printf(
		"[0x%02x]  0 1 2 3  4 5 6 7 01234567",
		(unsigned)address >> 24
	);

	// the system will halt during an memory access violation
	puts(
		"\n\n"
		"WARNING: KERNEL PANIC DUE TO\n\n"
		"MEMORY ACCESS VIOLATION\n\n\n\n"
		"PRESS THE RESET HOLE TO RECOVER"
	);
	gotoxy(0, 1);

	char *p = address;
	for (int row = 1; row < 10; row++) {
		// print last 6 bits in the memory address
		printf("%06x ", (unsigned)p & 0x00FFFFFF);

		for (int byte = 0; byte < 8; byte++, p++) {
			printf("%02x", *p);
			if ((byte & 0x3) == 0x3) {
				putchar(' ');
			}
		}
		p -= 8;

		for (int byte = 0; byte < 8; byte++, p++) {
			#define isprint(c) ((c) >= ' ')
			putchar(isprint(*p)? *p: '.');
		}
	}

	gotoxy(cursor, 10);
	putchar(0xAF);
	for (;;) {
		int key = get_key();
		if (key == 25) {
			return 0;  // exit program
		} else if (key == 20 || key == 22) {
			// [UP], [DOWN]
			int delta = (
				cursor == 5? 0x08:
				cursor == 4? 0x40:
				(1ull << (20 - cursor * 4))
			);
			address += (key - 21) * delta;
			return hex_viewer(address, cursor);
		} else {
			// [0] - [9], [A] - [F]
			if (0x0 <= key && key <= 0xF) {
				address = (int)address & ~(0xF << (20 - cursor * 4));
				address = (int)address |  (key << (20 - cursor * 4));
				key = 23;  // move to the next digit automatically
			}

			// [LEFT], [RIGHT]
			int next_cursor = cursor + key - 22;
			if (-2 <= next_cursor && next_cursor <= 5) {
				cursor = next_cursor;
			}
			return hex_viewer(address, cursor);
		}
	}
}
