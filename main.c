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
		while (any_key_pressed);
	}

	// [UP]: 0, [LEFT]: 1, [DOWN]: 2, [RIGHT]: 3
	if (col == 6) {
		return row + 20;
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


int
saturn_explorer(SAT_DIR_NODE *parent, SAT_DIR_NODE *node, SAT_DIR_ENTRY *entry)
{
	display_title(parent == __sat_root? "Saturn Explorer": parent->name);
	putchar('\n');

	if (!entry && !node) {
		node = parent->child;
	} else {
		hpg_set_indicator(HPG_INDICATOR_LSHIFT, 0xFF);
	}

	volatile unsigned count = 0;
	SAT_DIR_NODE *node_next_page = NULL;
	if (!entry) {
		for (SAT_DIR_NODE *n = node; n; n = n->sibling) {
			if (count == 8) {
				node_next_page = n;
				hpg_set_indicator(HPG_INDICATOR_RSHIFT, 0xFF);
				break;
			}
			count++;
			printf(" %2u %26s/\n", count, n->name);
		}
		entry = parent->object;
	}

	SAT_DIR_ENTRY *entry_next_page = NULL;
	if (!node_next_page) {
		for (SAT_DIR_ENTRY *e = entry; e; e = e->next) {
			if (count == 8) {/*
				entry_next_page = e;
				hpg_set_indicator(HPG_INDICATOR_RSHIFT, 0xFF);*/
				break;
			}
			count++;
			printf(
				" %2u %21s%6d\n",
				count, e->sat_obj->name, sat_strlen(e->sat_obj->addr)
			);
		}
	}

	for (;;) {
		int key = get_key();
		if (key == 27) {
			return 0;  // exit program
		} else if (key == 22 || key == 23) {
			if (node_next_page) {  // page down
				return saturn_explorer(parent, node_next_page, NULL);
			} else if (entry_next_page) {
				return saturn_explorer(parent, NULL, entry_next_page);
			}
		} else if (key == 20 || key == 21) {
			return saturn_explorer(parent, NULL, NULL);  // first page
		} else if (1 <= key && key <= count) {
			if (node) {
				for (SAT_DIR_NODE *n = node; n; n = n->sibling) {
					key--;
					if (!key) {
						return saturn_explorer(n, NULL, NULL);
					}
				}
			}
			for (SAT_DIR_ENTRY *e = entry; e; e = e->next) {
				key--;
				if (!key) {
					return object_viewer(parent, e->sat_obj);
				}
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
			return saturn_explorer(parent, NULL, NULL);  // go home
		}
	}
}
