/* writebmp.c -- crudely write a basic uncompressed BMP file.
 * Copyright 2021 Raymond D. Gardner
 */
#include <stdio.h>
#include <stdlib.h>

typedef unsigned long ulong;
typedef unsigned int uint;
typedef unsigned short ushort;

static void putbyte(int n, FILE *fp)
{
    fputc(n, fp);
}

static void putushort(ushort n, FILE *fp)
{
    putbyte(n & 0xFF, fp);
    putbyte((n >> 8) & 0xFF, fp);
}

static void putulong(ulong n, FILE *fp)
{
    putushort(n & 0xFFFF, fp);
    putushort((n >> 16) & 0xFFFF, fp);
}

void writebmp(char *fn, int width, int height, char *palette, uint palette_size, char *pixels)
{
    FILE *fp = fopen(fn, "wb");
    if (!fp) {
        printf("!! Cannot open %s -- quitting\n", fn);
        exit(1);
    }
    /* write file header */
    fwrite("BM", 1, 2, fp);
    //fwrite("    ", 1, 4, fp);   /* will be filesize */
    putulong(0, fp);    
    for (int k = 0; k < 4; k++)
        fwrite("", 1, 1, fp);
    //fwrite("    ", 1, 4, fp);   /* will be offset to pixel data */
    /* starts at offset 10 */
    putulong(0, fp);    
    //fwrite("    ", 1, 4, fp);   /* will be offset to pixel data */

    /* write image header */
    /* starts at offset 14 */
    putulong(40, fp);    
    putulong(width, fp);
    putulong(height, fp);
    putushort(1, fp);
    putushort(8, fp);
    putulong(0, fp);    
    putulong(0, fp);    /* will be image size in bytes */
    putulong(0, fp);    /* preferred x res pixels per meter */
    putulong(0, fp);    /* preferred y res pixels per meter */
    putulong(256, fp);    /* colors used */
    putulong(256, fp);    /* colors important */

    /* write color table (palette) */
    //fwrite(palette, 1, 768, fp);
#ifdef GDEBUG
fprintf(stderr, "palette_size:%d\n", palette_size);
#endif
    for (int k = 0; k < 256; k++) {
        uint r, g, b;
        if (k < palette_size) {
            r = palette[k*3];
            g = palette[k*3+1];
            b = palette[k*3+2];
        } else {
            r = g = b = 0;
        }
        putbyte(b, fp);
        putbyte(g, fp);
        putbyte(r, fp);
        putbyte(0, fp);
    }

    /* write pixel data */
    long pixeldataloc = ftell(fp);
    int pwidth = (width + 3) / 4 * 4;   /* covered quotient makes pwidth large enough multiple of 4 */

    for (int k = 0; k < height; k++) {
        /* write a row */
        int n;
        for (n = 0; n < width; n++)
            putbyte(*pixels++, fp);
        for (; n < pwidth; n++)
            putbyte(0, fp);
#if 0
    int wtf = 0;
    for (int k = 0; k < height; k++) {
        /* write a row */
        for (int n = 0; n < pwidth; n++) {
            wtf = ((wtf + 5) * 37) % 256;
            putbyte(wtf, fp);
        }
#endif
    }

    /* fixup */
    long flength = ftell(fp);
    fseek(fp, 2, SEEK_SET);
    putulong(flength, fp);
    fseek(fp, 10, SEEK_SET);
    putulong(pixeldataloc, fp);

    fseek(fp, 0, SEEK_END);
    fclose(fp);
}
