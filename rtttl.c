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

char repeat, paused;
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
		while (!SysCall(CheckBeepEndEntry) || paused) {
			if (alpha_pressed) {
				gotoxy(1, 7);
				puts((paused = !paused)? "||": "  ");
			} else if (shift_pressed) {
				set_indicator(INDICATOR_REMOTE, (repeat = !repeat));
			} else if (on_pressed) {
				paused = FALSE;
				SysCall(StopBeepEntry);
				return SUCCESS + 1;
			}
			while (any_key_pressed);
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
draw_line(unsigned row, int length)
{
	uint8_t *p = __display_buf + row * BYTES_PER_ROW + 2;
	int i;
	for (i = 0; i < length / 8; i++) {
		p[i] = 0xFF;
	}
	p[i] = 0xFF << (length & 0x7) >> 8;
}


void
init_progress_bar(SAT_STRING *str)
{
	for (int row = BAR_TOP; row < BAR_TOP + BAR_HEIGHT + 2; row++) {
		memset(&__display_buf[row * BYTES_PER_ROW], 0x00, 16);
		__display_buf[row * BYTES_PER_ROW + 1] = 0x80;  // left border
		__display_buf[row * BYTES_PER_ROW + 15] = 0x1;  // right border
	}
	draw_line(BAR_TOP, BAR_WIDTH + 1);  // top border
	draw_line(BAR_TOP + BAR_HEIGHT + 1, BAR_WIDTH + 1);  // bottom border

	char buf[7];
	utoa(str->end - str->begin, buf, 10);
	gotoxy(30 - strlen(buf), 6);
	puts(buf);
}


void
update_progress_bar(SAT_STRING *str, const char *melody)
{
	unsigned
		current = str->cursor - melody,
		total = str->end - melody,
		pixels = BAR_WIDTH * current / total;

	for (int row = BAR_TOP + 1; row < BAR_TOP + BAR_HEIGHT + 1; row++) {
		draw_line(row, pixels);  // progress bar
	}

	char buf[7];
	utoa(str->cursor - str->begin, buf, 10);
	gotoxy(4, 6);
	puts(buf);
}


void
raise(SAT_STRING *str)
{
	gotoxy(2, 3);
	puts(str->cursor >= str->end? "End of file": "Interrupted");
	puts("  Press any key to exit");
	while (any_key_pressed);
	while (!any_key_pressed);
	while (any_key_pressed);
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
	const char *melody = str->cursor;

begin:
	str->cursor = melody;
	init_progress_bar(str);

	while (TRUE) {
		update_progress_bar(str, melody);

		// get optional duration
		int duration = duration_parser(str);

		// convert ANSI note to MIDI note
		int note = 0;
		char c = toupper(peek(str));
		gotoxy(15, 6);
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
				for (volatile int i = 200 * duration; i && !on_pressed; i--) {
					if (alpha_pressed) {
						paused = TRUE;
					}
				}
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
		gotoxy(1, 7);
		puts("[]");
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
