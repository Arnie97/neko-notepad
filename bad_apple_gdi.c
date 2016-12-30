#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WIDTH 96
#define HEIGHT 64
#define SCALE 4
#define WIDTH_IN_BYTES (WIDTH / 8)

struct node_t {
	uint8_t byte;
	size_t weight;
	struct node_t *lchild, *rchild;
};

struct node_dump_t {
	uint16_t leaf: 1;
	uint16_t data: 9;
} pool[511];

uint16_t pool_size;

typedef struct node_t node_t;
typedef struct node_dump_t node_dump_t;
typedef const char *file_name_t;

void heap_push(node_t *);
node_t *heap_pop(void);

struct priority_queue {
	size_t len;
	node_t *data[256];
	void (*push)(node_t *);
	node_t *(*pop)(void);
} q = {
	.len = 0,
	.push = &heap_push,
	.pop = &heap_pop
};

struct {
	size_t count;  // character counter
	char *code;  // huffman coding
} alphabet[256];

uint8_t frame[HEIGHT][WIDTH_IN_BYTES];  // video buffer


void *
alloc(size_t size)
{
	void *p = malloc(size);
	if (p) {
		return p;
	} else {
		fputs("Failed to allocate memory\n", stderr);
		exit(EXIT_FAILURE);
	}
}


void
heap_push(node_t *n)
{
	int i = ++q.len, j;
	while ((j = i / 2)) {
		if (q.data[j]->weight <= n->weight) {
			break;
		}
		q.data[i] = q.data[j], i = j;
	}
	q.data[i] = n;
}


node_t *
heap_pop(void)
{
	if (!q.len) {
		return 0;
	}

	// save the smallest element
	q.data[0] = q.data[1];

	// remove the smallest element from the heap
	int i = 1, j;
	while ((j = i * 2) < q.len) {
		if (j + 1 < q.len && q.data[j + 1]->weight < q.data[j]->weight) {
			j++;
		}
		q.data[i] = q.data[j], i = j;
	}
	q.data[i] = q.data[q.len--];

	return q.data[0];
}


size_t
load_file(file_name_t name, uint8_t **pbuf)
{
	FILE *fp = fopen(name, "rb");
	if (!fp) {
		goto err;
	}

	fseek(fp, 0, SEEK_END);
	size_t len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	*pbuf = alloc(len);
	if (fread(*pbuf, 1, len, fp) != len) {
		goto err;
	}
	return len;

err:
	fprintf(stderr, "Failed to load file: %s\n", name);
	exit(EXIT_FAILURE);
}


void
delta_compress(file_name_t input, file_name_t intermediate)
{
	uint8_t *buf;
	size_t len = load_file(input, &buf);
	memset(frame, 0x00, HEIGHT * WIDTH_IN_BYTES);

	// create delta compressed file
	FILE *fp = fopen(intermediate, "wb");
	for (uint8_t *next = buf; next < buf + len; next += HEIGHT * WIDTH_IN_BYTES) {
		// convert pointer of the next frame to 2D array
		uint8_t (*array)[WIDTH_IN_BYTES] =
			(uint8_t (*)[WIDTH_IN_BYTES])next;

		for (int row = 0; row < HEIGHT; row++) {
			// row beginning mark
			uint8_t first_change_in_row = 0x80;

			for (uint8_t byte = 0; byte < WIDTH_IN_BYTES; byte++) {
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
}


node_t *
create_node(uint8_t byte, size_t weight, node_t *lchild, node_t *rchild)
{
	node_t *n = alloc(sizeof(node_t));
	if (lchild && rchild) {
		weight = lchild->weight + rchild->weight;
	}
	*n = (node_t){
		byte, weight, lchild, rchild
	};
	return n;
}


void
huffman_code(node_t *n, char *buf, size_t len)
{
	node_dump_t *self = &pool[pool_size++];
	if (!n->lchild || !n->rchild) {
		// leaf node, record character type
		*self = (node_dump_t){
			.leaf = TRUE,
			.data = n->byte
		};

		// copy huffman coding to alphabet
		alphabet[n->byte].code = alloc(len + 1);
		buf[len] = '\0';
		strcpy(alphabet[n->byte].code, buf);
	} else {
		// parent node, visit child nodes recursively
		// the left child must be located at *(self + 1) during a
		// preorder traversal, so doesn't need to be recorded
		buf[len] = '0'; huffman_code(n->lchild, buf, len + 1);

		// record right child
		*self = (node_dump_t){
			.leaf = FALSE,
			.data = pool_size - (self - pool)
		};

		buf[len] = '1'; huffman_code(n->rchild, buf, len + 1);
	}
	free(n);
}


uint8_t
generate_huffman_coding(uint8_t *buf, size_t len)
{
	// count each kind of character
	for (int i = 0; i < len; i++) {
		alphabet[buf[i]].count++;
	}

	// add non-zero entries to the priority queue
	uint8_t count = 0;
	for (int i = 0; i < 256; i++) {
		if (alphabet[i].count) {
			q.push(create_node(i, alphabet[i].count, NULL, NULL));
			count++;
		}
	}

	// build the huffman tree
	while (q.len > 1) {
		q.push(create_node(0, 0, q.pop(), q.pop()));
	}

	// generate huffman coding by preorder traversal
	// and serialize the huffman tree by the way
	char *huffman_code_buf = alloc(count);
	huffman_code(q.data[1], huffman_code_buf, 0);
	free(huffman_code_buf);

	return count;
}


void
huffman_compress(file_name_t intermediate, file_name_t output)
{
	uint8_t *buf;
	size_t len = load_file(intermediate, &buf);
	uint8_t count = generate_huffman_coding(buf, len);
	FILE *fp = fopen(output, "wb");

	// write the magic number
	fputs("\xBA\xDA\x99\x1E", fp);

	// write the serialized huffman tree
	fwrite(&pool_size, sizeof(uint16_t), 1, fp);
	fwrite(pool, sizeof(*pool), pool_size, fp);

	// write the encoded data
	char *output_buf = alloc(count + 8);
	output_buf[0] = '\0';

	for (uint8_t *p = buf; p < buf + len; p++) {
		// append huffman coding of the next character
		strcat(output_buf, alphabet[*p].code);

		// less than 8 bits in the buffer
		size_t output_len = strlen(output_buf);
		if (output_len < 8) {
			continue;
		}

		// otherwise, write bytes to the stream
		for (int i = 0; i + 8 <= output_len; i += 8) {
			uint8_t byte = 0x00;
			for (int j = 0; j < 8; j++) {
				byte <<= 1;
				byte |= (output_buf[i + j] == '1')? 1: 0;
			}
			fputc(byte, fp);
		}

		// move remaining bits to the front
		strcpy(output_buf, output_buf + (output_len & ~7u));
	}
	free(buf);

	{
		// write remaining bits into the last byte
		size_t output_len = strlen(output_buf);
		if (output_len) {
			uint8_t byte = 0x00;
			for (int j = 0; j < output_len; j++) {
				byte <<= 1;
				byte |= (output_buf[j] == '1')? 1: 0;
			}

			// add padding zeros
			byte <<= 8 - output_len;
			fputc(byte, fp);
		}
	}
	free(output_buf);
	fclose(fp);
}


void
draw(HDC hDC, const uint8_t *buf)
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


void
yield(uint8_t data, HDC hDC)
{
	static int row = -1, byte = -1;
	if (byte >= 0) {
		frame[row][byte] = data;
		byte = -1;
		return;
	}
	if (data != 0xFF) {
		byte = data & 0x7F;
	}
	if (data & 0x80) {
		row++;
		if (row == HEIGHT) {
			row = 0;
			draw(hDC, (uint8_t *)frame);
		}
	}
}


void
test_compressed_file(file_name_t output, HDC hDC)
{
	uint8_t *buf;
	size_t len = load_file(output, &buf);
	memset(frame, 0x00, HEIGHT * WIDTH_IN_BYTES);

	// skip magic number
	uint16_t pool_size = *(uint16_t *)(buf + 4);
	node_dump_t *tree = (node_dump_t *)(buf + 4 + sizeof(uint16_t)), *node = tree;
	for (uint8_t *p = (uint8_t *)tree + pool_size * sizeof(node_dump_t); p < buf + len; p++) {
		uint8_t byte = *p;
		for (int j = 0; j < 8; j++) {
			node += (byte & 0x80? node->data: 1);
			if (node->leaf) {
				yield(node->data, hDC);
				node = tree;
			}
			byte <<= 1;
		}
	}
	free(buf);
}


int
main(int argc, char *argv[])
{
	if (argc != 2 && argc != 4) {
		fprintf(stderr, "Usage: %s [input] [intermediate] output\n", argv[0]);
		exit(EXIT_FAILURE);
	} else if (argc == 4) {
		delta_compress(argv[1], argv[2]);
		huffman_compress(argv[2], argv[3]);
	}

	HWND hWnd = FindWindow("Notepad", NULL);
	if (hWnd) {
		// if notepad is found, play the compressed video in it
		HDC hDC = GetDC(hWnd);
		test_compressed_file(argv[argc - 1], hDC);
		ReleaseDC(hWnd, hDC);
	}
}
