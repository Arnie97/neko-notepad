/* Nibble-aligned Saturn strings

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

#ifndef _SATSTR_H
#define _SATSTR_H

typedef struct {
	const char *begin, *cursor, *end, aligned;
} SAT_STRING;

#define peek(str) ( \
	(str)->cursor >= (str)->end? ',': \
	(str)->aligned? \
	(str)->cursor[0]: \
	((str)->cursor[0] >> 4) + (char)((str)->cursor[1] << 4) \
)

int sat_strlen(unsigned sat_addr);
SAT_STRING sat_strdup(unsigned sat_addr);

#endif
