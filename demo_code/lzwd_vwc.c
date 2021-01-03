/* decompress LZW files
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#define MAXBYTE         255
#define CLEAR           256
#define END             257
#define MIN_CODE_WIDTH  9
#define CODE_LIMIT      4096
#define TABLE_SIZE      4096

typedef unsigned char Byte;
typedef unsigned int Uint;

Uint heads[TABLE_SIZE];
Byte tails[TABLE_SIZE];

Uint next_code;

Uint code_width;
Uint max_code;
Uint code_buffer = 0;
Uint bits_in_buf = 0;

FILE *inf, *outf;

int get_code()
{
    Uint code_bits_left, nbits;
    Uint code = 0;
    for (code_bits_left = code_width; code_bits_left; code_bits_left -= nbits) {
        if (bits_in_buf == 0) {
            code_buffer = getc(inf);
            bits_in_buf = 8;
        }
        nbits = bits_in_buf < code_bits_left ? bits_in_buf : code_bits_left;
        code |= (code_buffer & ~(-1 << nbits)) << (code_width - code_bits_left);
        code_buffer >>= nbits;
        bits_in_buf -= nbits;
    }
    return code;
}

void reset()
{
    next_code = 0;
    max_code = 2 * CLEAR - 1;
    code_width = MIN_CODE_WIDTH;
}

void decode()
{
    Uint code, in_code, prev_code, first_byte;
    Byte stack[CODE_LIMIT];
    Byte *stack_ptr = stack;

    reset();

    while ((code = get_code()) != END) {
        if (code == CLEAR) {
            reset();
        } else if (!next_code) {
            next_code = END + 1;
            first_byte = prev_code = code;
            assert(code <= MAXBYTE);
            putc(first_byte, outf);
        } else {
            in_code = code;
            /* Handle KwKwK case. */
            if (code >= next_code) {
                assert(code == next_code);
                *stack_ptr++ = first_byte;
                code = prev_code;
            }
            while (code > MAXBYTE) {
                *stack_ptr++ = tails[code];
                code = heads[code];
            }
            *stack_ptr++ = first_byte = code;
            do {
                putc(*--stack_ptr, outf);
            } while (stack_ptr != stack);
            if (next_code < CODE_LIMIT) {
                heads[next_code] = prev_code;
                tails[next_code++] = first_byte;
                if (next_code > max_code && next_code < CODE_LIMIT) {
                    max_code = max_code * 2 + 1;
                    code_width++;
                }
            }
            prev_code = in_code;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: decompress lzw_compressed_file orig_file\n");
        exit(0);
    }
    inf = fopen(argv[1], "rb");
    if (inf == NULL) {
        fprintf(stderr, "can't open input file %s\n", argv[1]);
        exit(1);
    }
    outf = fopen(argv[2], "wb");
    if (outf == NULL) {
        fprintf(stderr, "can't open output file %s\n", argv[2]);
        exit(1);
    }
    decode();
    fclose(outf);
    fclose(inf);
    exit(0);
}
