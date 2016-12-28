#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WIDTH 96
#define HEIGHT 64
#define SCALE 4
#define WIDTH_IN_BYTES (WIDTH / 8)

static unsigned char frame[HEIGHT][WIDTH_IN_BYTES];


long
load_file(const char *name, unsigned char **pbuf)
{
	FILE *fp = fopen(name, "rb");
	if (!fp) {
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	long len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	*pbuf = malloc(len);
	if (!*pbuf) {
		return 0;
	}

	if (fread(*pbuf, 1, len, fp) != len) {
		return 0;
	}
	return len;
}


void
draw(HDC hDC, const unsigned char *buf)
{
	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH_IN_BYTES; x++) {
			for (int b = 0; b < 8; b++) {
				COLORREF pixel = buf[y * WIDTH_IN_BYTES + x] & (1 << b)?
					RGB(0, 0, 0): RGB(255, 255, 255);
				SetPixelV(hDC, (x * 8 + b) * SCALE, y * SCALE, pixel);
			}
		}
	}
}


int
main(int argc, char *argv[])
{
	if (argc != 3) {
		puts("Usage: badapple [file] [compressed_file]");
		return 1;
	}

	unsigned char *buf;
	long len = load_file(argv[1], &buf);
	if (!len) {
		puts("Failed to load data file");
		return 2;
	}

	// create delta compressed file
	FILE *fp = fopen(argv[2], "wb");
	memset(frame, 0x00, HEIGHT * WIDTH_IN_BYTES);
	for (unsigned char *next = buf; next < buf + len; next += HEIGHT * WIDTH_IN_BYTES) {
		// convert pointer of the next frame to 2D array
		unsigned char (*array)[WIDTH_IN_BYTES] =
			(unsigned char (*)[WIDTH_IN_BYTES])next;

		for (unsigned row = 0; row < HEIGHT; row++) {
			// row beginning mark
			unsigned char first_change_in_row = 0x80;

			for (unsigned char byte = 0; byte < WIDTH_IN_BYTES; byte++) {
				// compare with corresponding byte in the previous frame
				if (frame[row][byte] != array[row][byte]) {
					frame[row][byte] = array[row][byte];

					// record row beginning
					putc(byte | first_change_in_row, fp);
					putc(array[row][byte], fp);
					first_change_in_row = 0x00;
				}
			}

			// add additional 0xFF as separator if a row isn't changed
			if (first_change_in_row) {
				putc(0xFF, fp);
			}
		}
	}
	free(buf);
	fclose(fp);

	// test the compressed file
	memset(frame, 0x00, 64 * 12);
	HWND hWnd = FindWindow("Notepad", NULL);
	if (!hWnd) {
		return 1;
	}
	HDC hDC = GetDC(hWnd);

	len = load_file(argv[2], &buf);
	for (unsigned row = -1, i = 0; i < len; i++) {
		if (buf[i] & 0x80) {
			row++;
			if (row == 64) {
				row = 0;
				draw(hDC, (unsigned char *)frame);

				static unsigned count;
				printf("%d\n", count++);
			}
		}
		if (buf[i] != 0xFF) {
			unsigned char byte = buf[i] & 0x7F;
			frame[row][byte] = buf[++i];
		}
	}

	ReleaseDC(hWnd, hDC);
}
