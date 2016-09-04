/* Implementation of Fletcher-16 checksum from Wikipedia

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


uint16_t
hash(const char *data)
{
	uint8_t sum1 = 0, sum2 = 0;
	while (*data) {
		sum1 = (sum1 + *data++) % 255;
		sum2 = (sum2 + sum1) % 255;
	}
	return (sum2 << 8) | sum1;
}


#ifndef __ARM_ARCH_4T__
#include <stdio.h>

int
main(int argc, const char *argv[])
{
	printf("%u\n", argc == 2? hash(argv[1]): 0);
}

#endif
