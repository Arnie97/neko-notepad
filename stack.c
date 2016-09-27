/* Stack that stores the addresses of previous pages

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
#include "hp39kbd.h"
#include "stack.h"


void
push(NODE **head, void *data)
{
	NODE *n = check_ptr(malloc(sizeof(NODE)));
	n->prev = *head;
	n->data = data;
	*head = n;
}


void *
pop(NODE **head)
{
	if (!*head) {
		return NULL;
	}
	NODE *n = *head;
	void *data = (*head)->data;
	*head = (*head)->prev;
	free(n);
	return data;
}


void *
check_ptr(void *p)
{
	if (p) {
		return p;
	}
	puts("Null Pointer!");
	for (;;) {
		get_key();
	}
}
