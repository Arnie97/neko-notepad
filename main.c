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
	const char *name = obj->name + (obj->name[0] == '\'');
	display_count(count, name);
	char buf[7];
	utoa(sat_strlen(obj->addr), buf, 10);
	for (unsigned i = 26 - strlen(name) - strlen(buf); i > 0; i--) {
		putchar(' ');
	}
	puts(buf);
}


int
note_explorer(SAT_DIR_ENTRY *init)
{
	display_title("Neko Music");

	if (!init) {
		__sat_cwd = _sat_find_path("/'notesdir");
		if (!__sat_cwd) {
			__sat_cwd = __sat_root;
		}
		init = __sat_cwd->object;
	} else {
		set_indicator(INDICATOR_LSHIFT, TRUE);
	}

	unsigned count = 0;
	SAT_OBJ_DSCR *entries[9];
	SAT_DIR_ENTRY *next_page = NULL;
	for (SAT_DIR_ENTRY *entry = init; entry; entry = entry->next) {
		SAT_OBJ_DSCR *obj = entry->sat_obj;
		if (sat_strlen(obj->addr) < 0) {
			continue;
		} else if (count == 8) {
			next_page = entry;
			set_indicator(INDICATOR_RSHIFT, TRUE);
			break;
		}
		count++;
		entries[count] = obj;
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
		} else if (1 <= key && key <= count) {
			return note_viewer(entries[key], head? head->data: NULL);
		}
	}
}


int
note_viewer(SAT_OBJ_DSCR *obj, SAT_DIR_ENTRY *ref)
{
	SAT_STRING str = sat_strdup(obj->addr);
	display_title(obj->name + (obj->name[0] == '\''));
	rtttl_parser(&str);
	return note_explorer(ref);  // go back to the list
}
