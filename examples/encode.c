/* encode.c -- encode GIF LZW files with giflzw lib
 * Copyright 2021 Raymond D. Gardner
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "glzwe.h"
#include "glzwd.h"

typedef unsigned int Uint;
typedef unsigned char Byte;

int main(int argc, char **argv)
{
    Uint incnt, incnt0, outcnt;
    enum {inbufsize = 30, outbufsize = 10};
    Byte inbuf[inbufsize], outbuf[outbufsize];
    FILE *infp = fopen(argv[1], "rb");
    assert(infp);
    FILE *outfp = fopen(argv[2], "wb");
    assert(outfp);
    void *state;
    int r = glzwe_init(&state, 8);
    Byte *p = inbuf;
    incnt = 0;
    int end_of_data = 0;
    do {
        outcnt = outbufsize;
        incnt0 = incnt;
        r = glzwe(state, p, outbuf, &incnt, &outcnt, end_of_data);
        fwrite(outbuf, 1, outbufsize - outcnt, outfp);
        if (r == GLZW_NO_INPUT_AVAIL) {
            p = inbuf;
            incnt = fread(p, 1, inbufsize, infp);
            end_of_data = incnt < inbufsize;
        } else if (r == GLZW_NO_OUTPUT_AVAIL) {
            p += incnt0 - incnt;
        } else if (r != GLZW_OK) {
            printf("ERROR r:%d\n", r);
            abort();
        }
    } while (r != GLZW_OK);
    fclose(infp);
    fclose(outfp);
    glzwe_end(state);
    return 0;
}
