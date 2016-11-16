#include <stdio.h>
#include <stdlib.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WIDTH 96
#define HEIGHT 64
#define SCALE 4

#define WIDTH_IN_BYTES (WIDTH / 8)


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
	if (argc != 2) {
		puts("Usage: badapple [file]");
		return 1;
	}

	unsigned char *buf;
	long len = load_file(argv[1], &buf);
	if (!len) {
		puts("Failed to load data file");
		return 2;
	}

	HWND hWnd = FindWindow("Notepad", NULL);
	HDC hDC = GetDC(hWnd);
	for (const unsigned char *p = buf; p < buf + len; p += WIDTH * HEIGHT / 8) {
		draw(hDC, p);
	}
	ReleaseDC(hWnd, hDC);
}
