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

#include <saturn.h>
#include "satstr.h"


int
sat_strlen(unsigned sat_addr)
{
	return sat_peek_sat_addr(sat_addr) == SAT_DOCSTR?
		((int)sat_peek_sat_addr(sat_addr + 5) - 5) / 2: -1;
}


SAT_STRING
sat_strdup(unsigned sat_addr)
{
	const char *arm_addr = sat_map_s2a(sat_addr + 10);
	SAT_STRING str = {
		.begin   = arm_addr,
		.cursor  = arm_addr,
		.end     = arm_addr + sat_strlen(sat_addr),
		.aligned = ~sat_addr & 1
	};
	return str;
}
