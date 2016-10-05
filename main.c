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

#include <saturn.h>
#include "satstr.h"
#include <hpconio.h>
#include <hpstring.h>
#include "hp39kbd.h"
#include "stack.h"
#include "display.h"
#include "main.h"

static unsigned font_id;


int
main(void)
{
	clear_screen();
	if (ROM->magic != 0xC0DEBA5E) {
		const char *rom_not_found = (
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
			memcpy(
				&__display_buf[(row + 18) * BYTES_PER_ROW],
				&rom_not_found[row * 11], 11
			);
		}
	} else if (hash(SERIAL_NO) != VALID_HASH) {
		size_t msg_len = strlen(ROM->anti_piracy) + strlen(SERIAL_NO);
		char *msg = check_ptr(malloc(msg_len));
		strcpy(msg, ROM->anti_piracy);
		strcat(msg, SERIAL_NO);
		SAT_STRING s = {
			.begin   = msg,
			.cursor  = msg,
			.end     = msg + msg_len,
			.aligned = TRUE
		};
		bitmap_blit(&s, ROM->fonts[0]);
	} else {
		return note_explorer(NULL);
	}
	for (;;) {
		get_key();
	}
}


int
event_handler(unsigned row, unsigned col)
{
	// [APLET], [HOME], [ON]
	if (row == 0 && (col == 7 || col == 4) || row == 6 && col == 6) {
		exit(col);
	} else {
		// wait until the key is released
		set_indicator(INDICATOR_WAIT, TRUE);
		while (any_key_pressed);
		set_indicator(INDICATOR_WAIT, FALSE);
	}

	// [UP]: 0, [LEFT]: 1, [DOWN]: 2, [RIGHT]: 3
	if (5 <= col && col <= 7) {
		return (col - 4) * 10 + row;
	} else if (3 <= row && row <= 5 && 1 <= col && col <= 3) {
		return (6 - row) * 3 - col + 1;
	}

	// unhandled keys
	return 0;
}


void
display_title(const char *str)
{
	unsigned len = strlen(str) + 2;
	extern int __scr_w;
	int right = (__scr_w - len) / 2, left = __scr_w - len - right;

	clear_screen();
	while (left--) {
		putchar('\x7f');
	}
	putchar(' ');
	while (*str) {
		putchar(*str++);
	}
	putchar(' ');
	while (right--) {
		putchar('\x7f');
	}
	for (int i = 0; i < 6; i++) {
		set_indicator(i, FALSE);
	}

	putchar('\n');
}


void
display_count(unsigned count, const char *name)
{
	putchar(' ');
	putchar(' ');
	putchar('[');
	putchar('0' + count);
	putchar(']');
	putchar(' ');

	while (*name) {
		putchar(*name++);
	}
}


void
display_item(unsigned count, SAT_OBJ_DSCR *obj)
{
	display_count(count, obj->name + (obj->name[0] == '\''));
	char buf[7];
	utoa(sat_strlen(obj->addr), buf, 10);
	for (unsigned i = 26 - strlen(obj->name) - strlen(buf); i > 0; i--) {
		putchar(' ');
	}
	puts(buf);
}


int
note_explorer(SAT_DIR_ENTRY *init)
{
	display_title("Neko Notepad");

	if (!init) {
		SAT_DIR_NODE *dir = _sat_find_path("/'notesdir");
		init = dir->object;
	} else {
		set_indicator(INDICATOR_LSHIFT, TRUE);
	}

	unsigned count = 0;
	SAT_DIR_ENTRY *next_page = NULL;
	for (SAT_DIR_ENTRY *entry = init; entry; entry = entry->next) {
		SAT_OBJ_DSCR *obj = entry->sat_obj;
		if (obj->name[0] == ';') {
			continue;
		}
		if (count == 8) {
			next_page = entry;
			set_indicator(INDICATOR_RSHIFT, TRUE);
			break;
		}
		count++;
		display_item(count, obj);
	}
	gotoxy(0, 9);

	static NODE *head;
	for (;;) {
		int key = get_key();
		if ((key == 22 || key == 23) && next_page) {
			push(&head, next_page);
			return note_explorer(next_page);  // page down
		} else if (key == 20 || key == 21) {
			pop(&head);
			return note_explorer(pop(&head));  // page up
		} else if (key == 31) {
			return font_config(head? head->data: NULL);
		} else if (1 <= key && key <= count) {
			for (SAT_DIR_ENTRY *entry = init; entry; entry = entry->next) {
				SAT_OBJ_DSCR *obj = entry->sat_obj;
				if (obj->name[0] == ';') {
					continue;
				}
				key--;
				if (!key) {
					return note_viewer(obj, head? head->data: NULL);
				}
			}
		}
	}
}


int
note_viewer(SAT_OBJ_DSCR *obj, SAT_DIR_ENTRY *ref)
{
	NODE *head = NULL;
	SAT_STRING str = sat_strdup(obj->addr);
	goto refresh;

	for (;;) {
		int key = get_key();
		if ((key == 22 || key == 23) && str.cursor != str.end) {
			refresh: push(&head, str.cursor);
			bitmap_blit(&str, ROM->fonts[font_id]);  // page down
			set_indicator(INDICATOR_LSHIFT, head->data != str.begin);
		} else if (key == 20 || key == 21) {
			pop(&head);
			str.cursor = pop(&head);  // page up
			if (!str.cursor) {
				str.cursor = str.begin;
			}
			goto refresh;
		} else if (key == 16) {  // [SYMB]
			while (head) {
				pop(&head);
			}
			return note_explorer(ref);  // go back to the list
		}
	}
}


int
font_config(SAT_DIR_ENTRY *ref)
{
	display_title("Select font size");

	int count = 0;
	while (ROM->fonts[count]) {
		char buf[3];
		utoa(ROM->fonts[count]->ROWS, buf, 10);
		display_count(++count, buf);
		puts(" px font");
	}
	for (;;) {
		int key = get_key();
		if (1 <= key && key <= count) {
			font_id = key - 1;
			key = 16;
		}
		if (key == 16 || key == 31) {  // [SYMB], [VIEWS]
			return note_explorer(ref);  // go back to the list
		}
	}
}
