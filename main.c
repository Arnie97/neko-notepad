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
#include <saturn.h>
#include <hpsys.h>
#include <hpstdio.h>
#include <hpconio.h>
#include <hpstring.h>
#include <hpgraphics.h>
#include "hp39kbd.h"
#include "main.h"


int
event_handler(unsigned row, unsigned col)
{
	// [APLET]
	if (row == 0 && col == 7) {
		// exit immediately
		return 27;
	} else {
		// wait until the key is released
		hpg_set_indicator(HPG_INDICATOR_WAIT, 0xFF);
		while (any_key_pressed);
		hpg_set_indicator(HPG_INDICATOR_WAIT, 0x00);
	}

	// [UP]: 0, [LEFT]: 1, [DOWN]: 2, [RIGHT]: 3
	if (col == 6) {
		return row + 20;
	} else if (row == 1 && col == 7) {
		return 28;  // [VIEWS]
	} else if (3 <= row && row <= 5 && 1 <= col && col <= 3) {
		return (6 - row) * 3 - col + 1;
	}

	// unhandled keys
	return 0;
}


inline int
sat_strlen(unsigned sat_addr)
{
	return ((int)sat_peek_sat_addr(sat_addr + 5) - 5) / 2;
}


inline char *
sat_strdup(unsigned sat_addr)
{
	unsigned len = sat_strlen(sat_addr);
	char *buf = sys_chkptr(malloc(len + 1));
	buf[len] = '\0';
	return sat_peek_sat_bytes(buf, sat_addr + 10, len);
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
		hpg_set_indicator(i, 0x00);
	}
}


SAT_DIR_ITEM
iterate_helper(SAT_DIR_NODE *parent, SAT_DIR_NODE *node, SAT_DIR_ENTRY *entry, int max, bool print)
{
	if (!entry && !node) {
		node = parent->child;
	} else {
		hpg_set_indicator(HPG_INDICATOR_LSHIFT, 0xFF);
	}

	unsigned count = 0;
	SAT_DIR_ITEM next_page = {.type = 0};
	if (!entry) {
		for (SAT_DIR_NODE *n = node; n; n = n->sibling) {
			if (count == max) {
				hpg_set_indicator(HPG_INDICATOR_RSHIFT, 0xFF);
				next_page.type = 1;
				next_page.data.node = n;
				return next_page;
			}
			count++;
			if (print) {
				printf(" %2u %26s/\n", count, n->name);
			}
		}
		entry = parent->object;
	}

	for (SAT_DIR_ENTRY *e = entry; e; e = e->next) {
		if (count == max) {
			hpg_set_indicator(HPG_INDICATOR_RSHIFT, 0xFF);
			next_page.type = 2;
			next_page.data.entry = e;
			return next_page;
		}
		count++;
		if (print) {
			printf(
				" %2u %21s%6d\n",
				count, e->sat_obj->name, sat_strlen(e->sat_obj->addr)
			);
		}
	}

	return next_page;
}


int
saturn_explorer(SAT_DIR_NODE *parent, SAT_DIR_NODE *node, SAT_DIR_ENTRY *entry)
{
	display_title(parent == __sat_root? "Saturn Explorer": parent->name);
	putchar('\n');

	SAT_DIR_ITEM next_page = iterate_helper(parent, node, entry, 8, 1);

	for (;;) {
		int key = get_key();
		if (key == 27) {
			return 0;  // exit program
		} else if (key == 22 || key == 23) {
			if (next_page.type == 1) {  // page down
				return saturn_explorer(parent, next_page.data.node, NULL);
			} else if (next_page.type == 2) {
				return saturn_explorer(parent, NULL, next_page.data.entry);
			}
		} else if (key == 20 || key == 21) {
			return saturn_explorer(parent, NULL, NULL);  // first page
		} else if (key == 26) {
			return saturn_explorer(__sat_root, NULL, NULL);  // go home
		} else if (key == 28 && parent != __sat_root) {
			return saturn_explorer(parent->parent, NULL, NULL);  // go back
		} else if (1 <= key && key <= 9) {
			SAT_DIR_ITEM child = iterate_helper(parent, node, entry, key - 1, 0);
			if (child.type == 1) {
				return saturn_explorer(child.data.node, NULL, NULL);
			} else if (child.type == 2) {
				return object_viewer(parent, child.data.entry->sat_obj);
			}
		}
	}
}


int
object_viewer(SAT_DIR_NODE *parent, SAT_OBJ_DSCR *obj)
{
	display_title(obj->name + 1);
	char *buf = sat_strdup(obj->addr);
	puts(buf);
	free(buf);

	for (;;) {
		int key = get_key();
		if (key == 27) {
			return 0;  // exit program
		} else if (key == 26) {
			return saturn_explorer(__sat_root, NULL, NULL);  // go home
		} else if (key == 28) {
			return saturn_explorer(parent, NULL, NULL);  // go back
		}
	}
}
