#!/usr/bin/env python3

import sys
import itertools


def msb(n):
    'Return position of the most significant bit in an integer.'
    i = 0
    while n:
        i += 1
        n >>= 1
    return i


for line in sys.stdin:
    group = line.split()

    if group[0] == 'FONT':
        print(' '.join(group[1:]))
    elif group[0] == 'STARTFONT':
        count = ascii_count = gb_count = 0
        storage = {}
    elif group[0] == 'ENDFONT':
        ascii_total, gb_total = 95, (6763 + 682)
        sys.stderr.write(
            '%d utilized glyphs amongst %d parsed ones.\n'
            ' ASCII coverage: %.2f%% (%d hits, %d misses)\n'
            'GB2312 coverage: %.2f%% (%d hits, %d misses)\n' % (
                ascii_count + gb_count, count,
                ascii_count / ascii_total * 100,
                ascii_count, ascii_total - ascii_count,
                gb_count / gb_total * 100,
                gb_count, gb_total - gb_count
            )
        )
        break
    elif group[0] == 'SIZE':
        pixel_size = int(group[1])
        hex_data_size = int((pixel_size + 7) / 8) * 8
    elif group[0] == 'STARTCHAR':
        glyph_name = group[1]
        count += 1

    elif group[0] == 'ENCODING':
        code_point = int(group[1])
        try:
            euc_cn = chr(code_point).encode('GB2312')
        except UnicodeEncodeError:
            while line != 'ENDCHAR':
                line = sys.stdin.readline().strip()
            continue
        except ValueError as e:
            print('Unknown glyph:', glyph_name)
            continue

        if code_point <= 0x7F:
            ascii_count += 1
            key = code_point
            charset = 'ASCII: %#X' % code_point
        else:
            gb_count += 1
            key = int.from_bytes(euc_cn, 'big')
            charset = 'EUC-CN: %#X  GB2312: %s' % (
                key,
                ''.join(format(byte - 0xA0, '02d') for byte in euc_cn)
            )
        print(
            'Glyph: "%s"  Unicode: %#X  %s' %
            (chr(code_point), code_point, charset)
        )

    elif group[0] == 'BBX':
        width, height, x0, y0 = map(int, group[1:5])
        print(width, height, x0, y0)

    elif group[0] == 'BITMAP':
        buffer, x_offset = [], 0
        while True:
            line = sys.stdin.readline().strip()
            if line != 'ENDCHAR':
                hex_data = int(line, 16)
                buffer.append(hex_data)
                x_offset = max(x_offset, msb(hex_data))

            else:
                glyph = []
                x_offset += x0 - pixel_size + 1
                y_offset = y0 + 1
                for i, _ in enumerate(buffer):
                    if x_offset > 0:
                        buffer[i] >>= x_offset
                    else:
                        buffer[i] <<= -x_offset
                    print(
                        format(buffer[i], '0%db' % width).
                        translate({ord('0'): '..', ord('1'): '[]'})
                    )
                buffer.extend(itertools.repeat(0, y_offset))

                storage[key] = buffer
                break


with open('UC1701_charset.h', 'w') as f:
    all_euc_cn_codepoints = itertools.product(
        itertools.chain(range(15, 87), range(9)),
        range(94))
    for x, y in all_euc_cn_codepoints:
        key = ((0xA1 + x) << 8) | (0xA1 + y)

        # Fill missing glyphs with zero
        in_buffer = storage.get(key, [0] * pixel_size)

        # Add padding for items with spaces
        while len(in_buffer) < pixel_size:
            in_buffer.insert(0, 0)

        transposed_buffer = [
            sum(
                ((byte << x_offset << 1 >> pixel_size) & 1) << y_offset >> 1
                for y_offset, byte in enumerate(in_buffer)
            )
            for x_offset in range(1, pixel_size)
        ]

        f.write('    {')
        for offset, byte in enumerate(transposed_buffer):
            if offset:
                f.write(', ')
            f.write('0x%02x' % byte)
        char = int.to_bytes(key, 2, 'big').decode('gb2312', 'ignore')
        f.write(
            '}, // [%02d%02d %04x %s] %s\n' %
            (x, y, key, ascii(char).replace("'", ''), char or 'FREE'))
