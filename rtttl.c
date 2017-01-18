/* RTTTL parser & buzzer driver

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

#include <syscall.h>
#include <hpconio.h>
#include <hpmath.h>
#include <hpstring.h>
#include "hp39kbd.h"
#include "display.h"

char repeat;
static int default_duration = 4, default_octave = 4, bpm = 120;


inline unsigned
freq(int note)
{
	return 110 * pow(2, (note - 69) / 12.);
}


int
beep_until_key_pressed(unsigned freq, unsigned duration, unsigned override)
{
	int ret = syscallArg3(beepEntry, freq, duration, override);
	if (ret == SUCCESS) {
		while (!SysCall(CheckBeepEndEntry)) {
			if (shift_pressed) {
				set_indicator(INDICATOR_REMOTE, (repeat = !repeat));
			} else if (any_key_pressed) {
				SysCall(StopBeepEntry);
				return SUCCESS + 1;
			}
		}
	}
	return ret;
}


int
parse_int(SAT_STRING *str)
{
	int x = 0;
	while (TRUE) {
		char c = peek(str);
		if (!isdigit(c)) {
			return x;
		}
		x = (x * 10) + (c - '0');
		str->cursor++;
	}
}


void
print_offset(SAT_STRING *str)
{
	char buf[7];
	utoa(str->cursor - str->begin, buf, 10);
	putchar(' ');
	putchar(' ');
	puts(buf);
}


void
raise(SAT_STRING *str)
{
	gotoxy(0, 2);
	putchar(' ');
	putchar(' ');
	puts(str->cursor >= str->end? "End of file": "Interrupted");
	puts("  Press any key to exit\n\n  At file position:");
	print_offset(str);
	while (any_key_pressed);
	while (!any_key_pressed);
}


int
duration_parser(SAT_STRING *str)
{
	int x = parse_int(str);
	return (60 * 1000 / bpm) * 4 / (x? x: default_duration);
}


void
melody_parser(SAT_STRING *str)
{
	const char *cursor = str->cursor;
	str->cursor = str->end;
	print_offset(str);
begin:
	str->cursor = cursor;

	while (TRUE) {
		gotoxy(0, 3);
		print_offset(str);
		putchar('\n');

		// get optional duration
		int duration = duration_parser(str);

		// convert ANSI note to MIDI note
		int note = 0;
		char c = toupper(peek(str));
		putchar(' ');
		putchar(' ');
		putchar(c);
		if ('A' <= c && c <= 'G') {
			const char notes[] = {9, 11, 0, 2, 4, 5, 7};
			note = notes[c - 'A'];

			// get optional sharp
			str->cursor++;
			if (peek(str) == '#') {
				str->cursor++;
				note++;
				putchar('#');
			} else {
				putchar(' ');
			}
		} else if (c == 'P') {
			str->cursor++;
			putchar(' ');
			putchar(' ');
		} else {
			break;
		}

		// get optional dotted note
		if (peek(str) == '.') {
			str->cursor++;
			duration += duration / 2;
		}

		// get optional octave
		if (c != 'P') {
			int x = parse_int(str);
			int octave = x? x: default_octave;
			note += (octave + 1) * 12;
			putchar(octave + '0');
		}

		if (peek(str) == ',') {
			if (note && SUCCESS != beep_until_key_pressed(freq(note), duration, 2)) {
				raise(str);
				return;
			} else if (!note) {
				sys_slowOn();
				for (volatile int i = 200 * duration; i && !any_key_pressed; i--);
				sys_slowOff();
			}
			str->cursor++;
		} else {
			break;
		}
	}

	if (repeat) {
		goto begin;
	} else {
		raise(str);
	}
}


void
rtttl_parser(SAT_STRING *str)
{
	while (str->cursor < str->end && peek(str) != ':') {
		str->cursor++;
	}
	str->cursor++;
	while (str->cursor < str->end) {
		char c = toupper(peek(str));
		if (c == 'D' || c == 'O' || c == 'B') {
			str->cursor++;
			if (peek(str) != '=') {
				break;
			}
			str->cursor++;
			int x = parse_int(str);
			if (x) {
				*(c == 'D'? &default_duration: c == 'O'? &default_octave: &bpm) = x;
			} else {
				raise(str);
				return;
			}
			if (peek(str) == ',') {
				str->cursor++;
			}
		} else if (c == ':') {
			str->cursor++;
			melody_parser(str);
			return;
		} else if (c == ' ' || c == '\n') {
			str->cursor++;
			continue;
		} else {
			break;
		}
	}
	raise(str);
}
