/* lzwe_clear.c -- encode LZW files - with CLEAR codes
 * Copyright 2021 Raymond D. Gardner
*/
#include <stdio.h>
#include <stdlib.h>


#define CLEAR       256
#define END         257
#define CODE_WIDTH  12
#define CODE_LIMIT  4096
#define TABLE_SIZE  4801
#define NOCODE      (-1)

typedef unsigned char Byte;
typedef unsigned int Uint;

Uint heads[TABLE_SIZE];
Byte tails[TABLE_SIZE];
Uint codes[TABLE_SIZE];

Uint next_code = END + 1;

Uint code_width = CODE_WIDTH;
Uint code_buffer = 0;
Uint buf_bits_left = 8;

FILE *inf, *outf;

void put_code(Uint code)
{
    Uint code_bits_left, nbits;

    for (code_bits_left = code_width; code_bits_left; code_bits_left -= nbits) {
        nbits = buf_bits_left < code_bits_left ? buf_bits_left : code_bits_left;
        code_buffer |= (code & ~(-1 << nbits)) << (8 - buf_bits_left);
        code >>= nbits;
        buf_bits_left -= nbits;
        if (buf_bits_left == 0) {
            putc(code_buffer, outf);
            code_buffer = 0;
            buf_bits_left = 8;
        }
    }
}

void flush()
{
    if (buf_bits_left)
        putc(code_buffer, outf);
}

int probe;

Uint lookup_in_table(Uint head, Uint tail)
{
    /* Hash found experimentally to be pretty good. */
    probe = (head * 211 + tail * 17) % TABLE_SIZE;
    while (heads[probe] != head || tails[probe] != tail) {
        if (heads[probe] == NOCODE)
            return NOCODE;
        /* Table size is prime; offset must be nonzero and < table size. */
        if ((probe -= (tail + 1)) < 0)
            probe += TABLE_SIZE;
    }
    return codes[probe];
}

void insert_in_table(Uint head, Uint tail, Uint code)
{
    heads[probe] = head;
    tails[probe] = tail;
    codes[probe] = code;
}
        
void reset()
{
    Uint i;
    for (i = 0; i < TABLE_SIZE; i++)
        heads[i] = NOCODE;
    next_code = END + 1;
}

void encode()
{
    Uint head, tail, code;
    reset();
    head = getc(inf);
    if (head != EOF) {
        while ((tail = getc(inf)) != EOF) {
            if ((code = lookup_in_table(head, tail)) != NOCODE) {
                head = code;
            } else {
                put_code(head);
                if (next_code < CODE_LIMIT) {
                    insert_in_table(head, tail, next_code++);
                } else {
                    put_code(CLEAR);
                    reset();
                }
                head = tail;
            }
        }
        put_code(head);
    }
    put_code(END);
    flush();
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: compress orig_file lzw_compressed_file\n");
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
    encode();
    fclose(outf);
    fclose(inf);
    return 0;
}
