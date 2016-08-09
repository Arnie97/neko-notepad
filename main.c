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
		return 5;
	} else {
		// wait until the key is released
		while (any_key_pressed);
	}

	// [UP]: 0, [LEFT]: 1, [DOWN]: 2, [RIGHT]: 3
	if (col == 6 && row < 4) {
		return row + 20;
	} else if (row <= 6 && col <= 4) {
		int ch = row * 5 - col + 'D';  // letter keys
		if (ch == 'D') {
			return 0;  // [DEL]
		} else if (ch < 'D') {
			ch++;  // skip the [DEL] key after [D]
		} else if (ch >= 'T') {
			ch--;  // skip the [ALPHA] key before [T]
			if (ch >= 'X') {
				ch--;  // skip the [SHIFT] key before [X]
				if (row == 6 && col == 0) {
					return 6;  // [ENTER]
				} else if (ch > 'Z') {
					return 0;
				}
			}
		}
		return ch;
	}

	// unhandled keys
	return 0;
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
		if (key == 5) {
			return 0;  // exit program
		} else if (key == 20 || key == 22) {
			address -= (key - 21) * (1ull << (20 - cursor * 4));
			return hex_viewer(address, cursor);
		} else if (key == 21 || key == 23) {
			int next_cursor = cursor + key - 22;
			if (-2 <= next_cursor && next_cursor <= 5) {
				return hex_viewer(address, next_cursor);
			}
		}
	}
}
