/* dumpgif.c -- GIF dump utility
 * Copyright 2021 Raymond D. Gardner
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* I don't know why #include <unistd.h> does not work in Linux. */
#include <getopt.h>
#include <assert.h>

#include "xdump.h"
#include "writebmp.h"
#include "crc32.h"
#include "glzwd.h"

char *usage_msg[] = {
"    dumpgif file.gif",
"    -p filename    write pixel data",
"    -z filename    write lzw data (de-blocked)",
"    -b filename    write a bmp file",

"    -g     hex dump global color table (GCT)",
"    -l     hex dump all local color tables (LCTs)",
    NULL
    };

#define OPT_PIXEL                   0x001
#define OPT_LZW                     0x002
#define OPT_BMP                     0x004
#define OPT_GCT                     0x008
#define OPT_LCT                     0x010

#define DUMP_SUBBLOCKS 0

typedef unsigned char Byte;
typedef unsigned int Word;
typedef unsigned long Dword;
size_t max_size_t = -1;

#define FATAL(x)        (printf x ,exit(1))

/* mmalloc: malloc; report and quit if error. */
void *mmalloc(size_t n)
{
    void *p = malloc(n);
    if (p == NULL) {
        FATAL(("!! malloc of %lu bytes failed. -- quitting\n.", (unsigned long)n));
    }
    return p;
}
/* mrealloc: realloc; report and quit if error. */
void *mrealloc(void *p, size_t n)
{
    void *newp = realloc(p, n);
    if (newp == NULL) {
        FATAL(("!! realloc of %lu bytes failed. -- quitting\n.", (unsigned long)n));
    }
    return newp;
}

#if 0
/* mstrdup: dup string; report and quit if error. */
char *mstrdup(char *s)
{
    size_t k = strlen(s) + 1;
    char *new_s = (char *)malloc(k);
    if (new_s == NULL) {
        FATAL(("!! malloc of %lu bytes failed in mstrdup. -- quitting\n.", k));
    }
    memmove(new_s, s, k);
    return new_s;
}
#endif

Word getword(Byte **p)
{
    Word v = ((*p)[1] << 8) | (*p)[0];
    //xdump((char *)*p, 2, 0x123);
    //printf("v: %u\n", v);
    (*p) += 2;
    return v;
}

Word getbyte(Byte **p)
{
    Byte v = (*p)[0];
    (*p) += 1;
    return v;
}

static int chsz = -1;
void lzd(char *px, Word image_size, char *lzwbuf, Word lzwbufcnt, Word lzw_min_code_size)
{
    void *st;
    Word in_avail = lzwbufcnt;
    Word out_avail = image_size;
    glzwd_init(&st, lzw_min_code_size);
    //printf("inited lz\n");
    if (chsz <= 0)
        chsz = lzwbufcnt;
    //fprintf(stderr, "chsz: %d\n", chsz);
    for (Word k = 0; k < lzwbufcnt; k+= chsz) {
//printf("k:%d\n", k);
        in_avail = chsz;
        if (lzwbufcnt - k < chsz)
            in_avail = lzwbufcnt - k;
        Word inavl0 = in_avail;
        Word outavl0 = out_avail;
        int r = glzwd(st, (Byte *)lzwbuf, (Byte *)px, &in_avail, &out_avail);
        lzwbuf += inavl0 - in_avail;
        px += outavl0 - out_avail;
        //fprintf(stderr, "r: %d %d\n", r, st.in_ptr-(Byte*)lzwbuf);
        if (r != GLZW_NO_INPUT_AVAIL)
            if (r)
                printf("LZW decode returned: %d\n", r);
    }
    //printf("done lz\n");
    glzwd_end(st);
}

Byte *do_image(int opts, char *infile, char *pixelfile, char *lzwfile, char
        *bmpfile, char *d, char *d_lim, Byte *p, Word background_color_index,
        Word global_color_table_flag, Word global_color_table_size, char *gct,
        int transparent_color_index)
{
    printf("Image Descriptor at 0x%0lx:\n", (Dword)((char *)p - d));
    Word image_left_pos = getword(&p);
    Word image_top_pos = getword(&p);
    Word image_width = getword(&p);
    Word image_height = getword(&p);
    size_t image_size = image_width * image_height;
    printf("image: left:%u  top:%u  width:%u  height:%u\n", image_left_pos,
            image_top_pos, image_width, image_height);
    //printf("imgpacked offset: 0x%0lx\n", (Dword)((char *)p - d));
    Word packed = getbyte(&p);
    //printf("imgpacked: 0x%0x\n", packed);
    Word local_color_table_flag = (packed >> 7) & 0x1;
    Word interlace_flag = (packed >> 6) & 0x1;
    Word sort_flag = (packed >> 5) & 0x1;
    Word reserved = (packed >> 3) & 0x3;
    Word local_color_table_size_base = packed & 0x7;
    Word local_color_table_size = local_color_table_flag ? 1 <<
        (local_color_table_size_base + 1) : 0;
    //printf("lct_flag: %u interlace: %u sort: %u rsv: %u lct_size: %u (%u)\n",
    printf("lct_flag:%u  interlace:%u  sort:%u  rsv:%u  lct_size:%u (%u)",
        local_color_table_flag, interlace_flag, sort_flag, reserved,
        local_color_table_size_base, local_color_table_size);

    /* local color table (optional) */
    char *lct = "";
    if (local_color_table_flag) {
        lct = (char *)mmalloc(3 * local_color_table_size);
        memmove(lct, p, 3 * local_color_table_size);
        Dword crc = crc32(lct, local_color_table_size, 0);
        printf(" LCT CRC: %08x\n", (Word)crc);
        if (opts & OPT_LCT) {
            printf("Local Color Table (LCT):\n");
            xdump(lct, 3 * local_color_table_size, (char *)p - d);
        }
        p += 3 * local_color_table_size;
    } else {
        //printf("No Local Color Table\n");
        printf("\n");
    }

    if (transparent_color_index >= 0) {
        char *ct = local_color_table_flag ? lct : gct;
        Word ctsize = local_color_table_flag ? local_color_table_size : global_color_table_size;
        if (transparent_color_index > ctsize) {
            printf("!! INVALID transparent color index: %u\n", transparent_color_index);
            transparent_color_index = 0;
        }

        Byte *t = (Byte *)ct + 3 * transparent_color_index;
        Word tc = (t[0] << 16) + (t[1] << 8) + t[2];
        printf("transparent color: 0x%06x\n", tc);
    }

    printf("Image Data at 0x%0lx:\n", (Dword)((char *)p - d));
    Word lzw_min_code_size = getbyte(&p);
    printf("LZW init code size:%u\n", lzw_min_code_size);
    char *lzwbuf = (char *)mmalloc(image_size);
    char *t = lzwbuf;
    char *tlimit = lzwbuf + image_size;
    /* lzwbuf is as large as image size; since it's "compressed" it should
     * be large enough to hold the compressed data of the image. This may
     * not be the case, so we will check for space and use realloc
     * if needed.
     * */
    unsigned char *p0 = p;
    for (;;) {
        Word subblocksize = getbyte(&p);
#if DUMP_SUBBLOCKS
        printf("sbl: %u\n", subblocksize);
#endif
        if (!subblocksize)
            break;
#if DUMP_SUBBLOCKS
        xdump((char *)p, subblocksize, 0x234);
#endif
        if (t + subblocksize > tlimit) {
            /* TODO: check this out; test if possible.
             * Need special case GIF to test it.
             * */
            size_t toffset = t - lzwbuf;
            size_t new_bufsize = tlimit - lzwbuf + subblocksize;
            lzwbuf = (char *)mrealloc(lzwbuf, new_bufsize);
            t = lzwbuf + toffset;
            tlimit = lzwbuf + new_bufsize;
        }
        memmove(t, p, subblocksize);
        t += subblocksize;
        p += subblocksize;
    }

    printf("LZW data size:%lu  image_size:%lu  net data size:%lu\n",
        (Dword)(p - p0), (Dword)image_size, (Dword)(t - lzwbuf));
    int todump = p - p0;
#if DUMP_RAW_LZW
    if (todump > 64) todump = 64;
    xdump((char *)p0, todump, (char *)p - d);
#endif
    todump = t - lzwbuf;
    if (opts & OPT_LZW) {
        FILE *fp = fopen(lzwfile, "wb");
        assert(fp);
        printf("Writing %d lzw from %p\n", todump, lzwbuf);
        fwrite(lzwbuf, 1, todump, fp);
        fclose(fp);
    }
#if DUMP_NET_LZW
    if (todump > 64) todump = 64;
    xdump((char *)lzwbuf, todump, 0);
#endif
    char *px = (char *)mmalloc(image_size);
    memset(px, 0xba, image_size);
    lzd(px, image_size, lzwbuf, t - lzwbuf, lzw_min_code_size);
    //xdump((char *)px, image_size, 0);
    todump = image_size;
    if (opts & OPT_PIXEL) {
        FILE *fp = fopen(pixelfile, "ab");
        assert(fp);
        printf("writing %d px from %p\n", todump, px);
        fwrite(px, 1, todump, fp);
        fclose(fp);
    }
#if DUMP_PIXELS
    printf("Pixel data:\n");
    if (todump > 64) todump = 64;
    xdump((char *)px, todump, 0);
#endif
    free(lzwbuf);

    if (opts & OPT_BMP) {
        writebmp(bmpfile, image_width, image_height, gct,
                global_color_table_size, px);
    }
    free(px);
    return p;
}

void dumpgif(int opts, char *infile, char *pixelfile, char *lzwfile,
        char *bmpfile, char *d, char *d_lim)
{
    Byte *p = (Byte *)d;
    //xdump(d, bufsize, 0);
    /* header */
    char sig[4] = "   ", version[4] = "   ";
    memmove(sig, p, 3);
    p += 3;
    memmove(version, p, 3);
    p += 3;
    if (strcmp(sig, "GIF") != 0) {
        printf("NOT A GIF\n");
        return;
    }
    printf("sig:{%s}  version:{%s}\n", sig, version);
    Word ver = 0;
    if (!strcmp(version, "87a"))
        ver = 87;
    if (!strcmp(version, "89a"))
        ver = 89;
    if (ver != 87 && ver != 89)
        printf("!! UNKNOWN VERSION {%s}\n", version);
    /* logical screen descriptor */
    printf("Logical Screen Descriptor (LSD):\n");
    Word logical_screen_width = getword(&p);
    Word logical_screen_height = getword(&p);
    Word packed = getbyte(&p);
    //printf("packed: %x %u\n", packed, packed);
    Word global_color_table_flag = (packed >> 7) & 0x1;
    Word color_resolution = (packed >> 4) & 0x7;
    Word sort_flag = (packed >> 3) & 0x1;
    Word global_color_table_size_basis = packed & 0x7;
    Word background_color_index = getbyte(&p);
    Word pixel_aspect_ratio = getbyte(&p);
    Word global_color_table_size = global_color_table_flag ? 1 <<
        (global_color_table_size_basis + 1) : 0;

    /* global color table (optional) */
    char *gct = "";
    if (global_color_table_flag) {
        gct = (char *)mmalloc(3 * global_color_table_size);
        memmove(gct, p, 3 * global_color_table_size);

        if (background_color_index > global_color_table_size) {
            printf("!! INVALID background color index: %u\n", background_color_index);
            background_color_index = 0;
        }
        Byte *bg = (Byte *)gct + 3 * background_color_index;
        Word bgc = (bg[0] << 16) + (bg[1] << 8) + bg[2];
        printf("width:%u  height:%u  bg index:%u (0x%06x) aspect ratio:%u  GCT flag:%u  res:%u (%u)  sort:%u  GCT size:%u (%u)\n",
            logical_screen_width, logical_screen_height,
            background_color_index, bgc, pixel_aspect_ratio,
            global_color_table_flag,
            color_resolution, 1 << (color_resolution + 1),
            sort_flag, global_color_table_size_basis, global_color_table_size);
    } else {
        printf("width:%u  height:%u  bg index:%u  aspect ratio:%u  GCT flag:%u  res:%u (%u)  sort:%u  GCT size:%u (%u)\n",
            logical_screen_width, logical_screen_height,
            background_color_index, pixel_aspect_ratio,
            global_color_table_flag,
            color_resolution, 1 << (color_resolution + 1),
            sort_flag, global_color_table_size_basis, global_color_table_size);
    }

    if (global_color_table_flag) {
        Dword crc = crc32(gct, global_color_table_size, 0);
        printf("GCT CRC: %08x\n", (Word)crc);
        if (opts & OPT_GCT) {
            printf("Global Color Table (GCT) at 0x%0lx:\n", (Dword)((char *)p - d));
            xdump(gct, 3 * global_color_table_size, (char *)p - d);
        }
        p += 3 * global_color_table_size;
    }

    /* data (optional) */
    /* ([Graphic Control Extension] ( (Image Descriptor [Local Color Table] Image Data) | Plain Text Extension) | <Special-Purpose Block>)*
    */

    int got_trailer = 0;
    int transparent_color_index = -1;
    Word xtype;
    for (;;) {
        int id = getbyte(&p);
        printf("block id: 0x%0x\n", id);
        switch (id) {
            case 0x3B:  /* trailer */
                        got_trailer = 1;
                        printf("TRAILER\n");
                        break;
            case 0x2C:  /* image descriptor */
                        //p = do_image(infile, d, d_lim, p);

                        p = do_image(opts,
                                infile,
                                pixelfile, lzwfile, bmpfile,
                                d, d_lim, p,
                                background_color_index, global_color_table_flag,
                                global_color_table_size, gct,
                                transparent_color_index);

                        transparent_color_index = -1;
                        break;
            case 0x21:  /* extension introducer */
                        /* extensions */
                        xtype = getbyte(&p);
                        if (xtype == 0xF9) { /* graphic control extension */
                            Word blocksize = getbyte(&p);
                            //printf("Graphic Control Extension: %u bytes\n", blocksize);
                            printf("Graphic Control Extension at 0x%0lx: %u bytes\n",
                                    (Dword)((char *)p - d - 1), blocksize);
                            xdump((char *)p, blocksize, 0x0);
                            Word packed = getbyte(&p);
                            Word disposal_method = (packed >> 2) & 0x7;
                            Word user_input_flag = (packed >> 1) & 0x1;
                            Word transparent_color_flag = packed & 0x1;
                            Word delay_time = getword(&p);
                            transparent_color_index = getbyte(&p);
                            printf("disposal method:%u  user input flag:%u  transparent color flag:%u",
                                    disposal_method, user_input_flag, transparent_color_flag);
                            printf("  delay_time:%u  transparent color index:%u\n",
                                    delay_time, transparent_color_index);
                            //p += blocksize;
                            if (! transparent_color_flag)
                                transparent_color_index = -1;
                            Word terminator = getbyte(&p);
                            /* TODO fix this: */
                            assert(terminator == 0);
                        } else if (xtype == 0xFE) {      /* comment extension */
                            for (;;) {
                                Word subblocksize = getbyte(&p);
                                printf("\nComment Extension: %u bytes\n", subblocksize);
                                if (!subblocksize)
                                    break;
                                xdump((char *)p, subblocksize, 0x0);
                                printf("%.*s", subblocksize, p);
                                p += subblocksize;
                            }
                            printf("\n");
                        } else if (xtype == 0x01) { /* plain text extension */
                            Word blocksize = getbyte(&p);
                            printf("Plain Text Extension: %u bytes\n", blocksize);
                            xdump((char *)p, blocksize, 0x0);
                            p += blocksize;
                        } else if (xtype == 0xFF) { /* application extension */
                            Word blocksize = getbyte(&p);
                            printf("Application Extension: %u bytes\n", blocksize);
                            xdump((char *)p, blocksize, 0x0);
                            //char axid[9] = "        ";
                            //memmove(axid, p, 8);
                            p += blocksize;
                            //FILE *fp = fopen(axid, "wb");
                            for (;;) {
                                Word subblocksize = getbyte(&p);
                                printf("\nApplication Data: %u bytes\n", subblocksize);
                                if (!subblocksize)
                                    break;
                                xdump((char *)p, subblocksize, 0x0);
                                //fwrite(p, 1, subblocksize, fp);
                                //printf("%.*s", subblocksize, p);
                                p += subblocksize;
                            }
                            //fclose(fp);
                            printf("\n");
                        } else { /* plain text extension */
                            printf("!!UNKNOWN EXTENSION!!\n");
                            /* TODO how to handle? */
                        }
                        break;
            default:
                        break;
        }
        if (got_trailer)
            break;
    }
}
    /*
    GIF grammar (refactored):
    <GIF Data Stream> ::= Header Logical_Screen_Descriptor [Global_Color_Table]
    (
        (
            [Graphic_Control_Extension]
            (
                Image_Descriptor [Local_Color_Table] Image_Data
                | Plain_Text_Extension
            )
        )
        | Application_Extension
        | Comment_Extension
    )*
    Trailer


    GIF grammar (original):
    <GIF Data Stream> ::= Header <Logical Screen> <Data>* Trailer
    <Logical Screen> ::= Logical Screen Descriptor [Global Color Table]
    <Data> ::= <Graphic Block> | <Special-Purpose Block>
    <Graphic Block> ::= [Graphic Control Extension] <Graphic-Rendering Block>
    <Graphic-Rendering Block> ::= <Table-Based Image> | Plain Text Extension
    <Table-Based Image> ::= Image Descriptor [Local Color Table] Image Data
    <Special-Purpose Block> ::= Application Extension | Comment Extension
    */

void do_file(int opts, char *infile, char *pixelfile, char *lzwfile, char *bmpfile)
{
    FILE *fp;
    long flength;
    size_t bufsize;
    char *d;
    long r;
    printf("File: %s\n", infile);
#ifdef GDEBUG
fprintf(stderr, "File: %s\n", infile);
#endif
    fp = fopen(infile, "rb");
    if (!fp)
        FATAL(("!! Cannot open %s -- quitting\n", infile));
    r = fseek(fp, 0, SEEK_END);
    assert(r == 0);

    flength = ftell(fp);
    rewind(fp);
    if (flength > max_size_t)
        FATAL(("!! File %s too large to read\n", infile));
    bufsize = flength;
    d = (char *)mmalloc(bufsize);
    r = fread(d, 1, bufsize, fp);
    assert(r == bufsize);
    fclose(fp);
    dumpgif(opts, infile, pixelfile, lzwfile, bmpfile, d, d + bufsize);
    free(d);
}

void usage(char **msg)
{
    char **p;
    for (p = msg; *p; p++)
        printf("%s\n", *p);
    exit(1);
}

int main(int argc, char **argv)
{
    int opts = 0;
    int c;
    opterr = 0;

    char *infile = NULL, *pixelfile = NULL, *lzwfile = NULL, *bmpfile = NULL;

    while ((c = getopt(argc, argv, "hp:z:b:gl")) != -1) {
        switch (c) {
            case 'h':
                usage(usage_msg);
            case 'p':
                opts |= OPT_PIXEL;
                pixelfile = optarg;
                break;
            case 'z':
                opts |= OPT_LZW;
                lzwfile = optarg;
                break;
            case 'b':
                opts |= OPT_BMP;
                bmpfile = optarg;
                break;
            case 'g':
                opts |= OPT_GCT;
                break;
            case 'l':
                opts |= OPT_LCT;
                break;
        }
    }
    infile = argv[optind];
    if (!infile) {
        printf("Missing input filename.\n");
        usage(usage_msg);
    }
    do_file(opts, infile, pixelfile, lzwfile, bmpfile);
    return 0;
}
