/* runlzw.c -- test/exercise giflzw lib routines.
 * Copyright 2021 Raymond D. Gardner
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* I don't know why #include <unistd.h> does not work in Linux. */
#include <getopt.h>
#include <assert.h>

#include "adler32.h"
#include "glzwe.h"
#include "glzwd.h"
#include "xdump.h"


char *usage_msg[] = {
"    rr -n number or -f infile",
"    -o outfile",
"    -x dumpfilename dump random numbers or possibly masked input file",
"    -b nbits (1-8) will be masked on random numbers or input file",
"",
"    -e encode infile to outfile",
"    -d decode infile to outfile",
"    -r encode and/or decode random chunks",
"",
"    -E only encode",
"    -g only encode",
"    -P print encoded codes",
"    -D print decoded codes",
"    -h print usage message",
    NULL
    };

#define OPT_RANDOM                  0x001
#define OPT_INFILE                  0x002
#define OPT_OUTFILE                 0x004
#define OPT_NBITS                   0x008
#define OPT_ENCODE                  0x010
#define OPT_DECODE                  0x020
#define OPT_DUMPINPUT               0x040
#define OPT_RANDOM_CHUNKS           0x080
#define OPT_ONLY_ENCODE             0x100
#define OPT_PRINT_ENCODED_CODES     0x200
#define OPT_PRINT_DECODED_CODES     0x400

extern int nsuccesses, nfails, nreprobes, ninserts;
extern int print_enc_codes, print_dec_codes;

typedef unsigned char Byte;
typedef unsigned int Uint;

#define minout 100

/*
#define OPT_OUTFILE                 0x004
#define OPT_ENCODE                  0x010
#define OPT_DECODE                  0x020
#define OPT_ONLY_ENCODE             0x100
#define OPT_PRINT_ENCODED_CODES     0x200
#define OPT_PRINT_DECODED_CODES     0x400
 */
#define ndeb 0
void run_single_chunk(int opts, int nbits, char *outfile, int n, Byte *p)
{
    int i, r;
    unsigned long adlersum = adler32(p, n);
#if ndeb
    printf("adler: %08x\n", (Uint)adlersum);
#endif
    if(n<10)xdump(p, n, 0);
    /* LZW 12-bit should not expand beyond 12:8, but
     * may for very small files due to CLEAR/END overhead.
     * Allow for a reasonable minimum.
     */
    int n_enc = 3 * n / 2;
    if (n_enc < minout)
        n_enc = minout;
    Byte *enc_buf = (Byte *)malloc(n_enc);
    assert(enc_buf);
    Byte *enc_buf_init = enc_buf;
    Uint end_of_data = 1;
    Uint lzw_min_code_size = nbits < 2 ? 2 : nbits;
    void *encoder_state;

    Uint in_avail = n;
    Uint out_avail = n_enc;
    clock_t nticks = clock();
    r = glzwe_init(&encoder_state, lzw_min_code_size);
    assert(r == 0);
    r = glzwe(encoder_state, p, enc_buf, &in_avail, &out_avail, end_of_data);
    if (r) {
        printf("ERROR: glzwe returned %d\n", r);
        return;
    }
    glzwe_end(encoder_state);
    nticks = clock() - nticks;
    printf("done encoding: glzwe returned %d\n", r);
    printf("%8d succeeded\n%8d failed\n%8d reprobes\n%8d inserts\n",
    nsuccesses, nfails, nreprobes, ninserts);
    Uint enc_size = n_enc - out_avail;
    printf("encoded size: %d (%.3f%%)\n", enc_size, enc_size / (float)n);

    long millisecs = (nticks * 1000L) / (long)(CLOCKS_PER_SEC);
    if ( nticks < 0 )
        printf("***timer error: %ld\n", -nticks);
    else
        printf("encoded in: %ld.%02ld sec\n",
            millisecs/1000, ((millisecs%1000)+5)/10);

    if(n<10)xdump(enc_buf, enc_size, 0);

    if (opts & OPT_ENCODE) {
        FILE *fp = fopen(outfile, "wb");
        if (!fp) {
            printf("can't open file %s\n", outfile);
            exit(1);
        }
        size_t k = fwrite(enc_buf_init, 1, enc_size, fp);
        assert(k == enc_size);
        fclose(fp);
    }

    if (opts & OPT_ONLY_ENCODE)
        return;

    void *decoder_state;
    Byte *dec_buf = (Byte *)malloc(n);
    assert(dec_buf);
    in_avail = enc_size;
    out_avail = n;

    nticks = clock();
    r = glzwd_init(&decoder_state, lzw_min_code_size);
    assert(r == 0);
    r = glzwd(decoder_state, enc_buf, dec_buf, &in_avail, &out_avail);
    if (r) {
        printf("ERROR: glzwd returned %d\n", r);
        return;
    }
    glzwd_end(decoder_state);
    nticks = clock() - nticks;

    assert(out_avail == 0);
    int dec_size = n;
    printf("decoded size: %d\n", dec_size);
#if ndeb
    printf("adler: %08x\n", (Uint)adler32(dec_buf, dec_size));
#endif
    millisecs = (nticks * 1000L) / (long)(CLOCKS_PER_SEC);
    if ( nticks < 0 )
        printf("***timer error: %ld\n", -nticks);
    else
        printf("decoded in: %ld.%02ld sec\n",
            millisecs/1000, ((millisecs%1000)+5)/10);
    assert(n == dec_size);
    assert(adlersum == adler32(dec_buf, dec_size));
#if 1
    for (i = 0; i < dec_size; i++)
        assert(p[i] == dec_buf[i]);
#endif
}

void run_random_chunks(int opts, int nbits, char *outfile, int n, Byte *p)
{
    int i, r;
    unsigned long adlersum = adler32(p, n);
#if 0
    printf("adler: %ld\n", adlersum);
#endif
    if(n<10)xdump(p, n, 0);
    /* LZW 12-bit should not expand beyond 12:8, but
     * may for very small files due to CLEAR/END overhead.
     * Allow for a reasonable minimum.
     */
    int n_enc = 3 * n / 2;
    if (n_enc < minout)
        n_enc = minout;
    Byte *enc_buf = (Byte *)malloc(n_enc);
    assert(enc_buf);
    Byte *p_init = p;
    Byte *enc_buf_init = enc_buf;
    Uint end_of_data = 0;
    Uint lzw_min_code_size = nbits < 2 ? 2 : nbits;
    void *encoder_state;

    clock_t nticks = clock();
    r = glzwe_init(&encoder_state, lzw_min_code_size);
    assert(r == 0);
    /* Break and output into random chunks of 0-4 and 0-6 bytes. */
    Uint in_avail = rand() % 5;
            if (in_avail >= n) {
                in_avail = n;
                end_of_data = 1;
            }
    Uint out_avail = rand() % 7;
    int ncalls = 0;
    Uint in_avail_used_tot = 0;
    Uint out_avail_used_tot = 0;
    for (;;) {
        ncalls++;
        Uint in_avail_prev = in_avail;
        Uint out_avail_prev = out_avail;
        r = glzwe(encoder_state, p, enc_buf, &in_avail, &out_avail, end_of_data);
        Uint in_used = in_avail_prev - in_avail;
        p += in_used;
        Uint out_used = out_avail_prev - out_avail;
        enc_buf += out_used;
        in_avail_used_tot += in_used;
        out_avail_used_tot += out_used;

        if (r == GLZW_OK)
            break;
        if (r == GLZW_NO_INPUT_AVAIL) {
            in_avail = rand() % 5;
            int lefttogo = n - in_avail_used_tot;
            if (in_avail >= lefttogo) {
                in_avail = lefttogo;
                end_of_data = 1;
            }
            continue;
        } else if (r == GLZW_NO_OUTPUT_AVAIL) {
            out_avail = rand() % 7;
            continue;
        } else {
            printf("ERROR: glzwe returned %d\n", r);
            return;
        }
    }
    glzwe_end(encoder_state);
    nticks = clock() - nticks;
    printf("done encoding: glzwe called %d times\n", ncalls);
    printf("%8d succeeded\n%8d failed\n%8d reprobes\n%8d inserts\n",
    nsuccesses, nfails, nreprobes, ninserts);
    Uint enc_size = enc_buf - enc_buf_init;
    assert(enc_size == out_avail_used_tot);
    /*assert(enc_buf - enc_buf_init == out_avail_used_tot);
     */
    printf("encoded size: %d (%.3f%%)\n", enc_size, enc_size / (float)n);

    long millisecs = (nticks * 1000L) / (long)(CLOCKS_PER_SEC);
    if ( nticks < 0 )
        printf("***timer error: %ld\n", -nticks);
    else
        printf("encoded in: %ld.%02ld sec\n",
            millisecs/1000, ((millisecs%1000)+5)/10);
    if(n<10)xdump(enc_buf_init, enc_size, 0);

    if (opts & OPT_ENCODE) {
        FILE *fp = fopen(outfile, "wb");
        if (!fp) {
            printf("can't open file %s\n", outfile);
            exit(1);
        }
        size_t k = fwrite(enc_buf_init, 1, enc_size, fp);
        assert(k == enc_size);
        fclose(fp);
    }

    if (opts & OPT_ONLY_ENCODE)
        return;

    void *decoder_state;
    enc_buf = enc_buf_init;
    Byte *dec_buf = (Byte *)malloc(n);
    assert(dec_buf);
    Byte *dec_buf_init = dec_buf;

    nticks = clock();
    r = glzwd_init(&decoder_state, lzw_min_code_size);
    assert(r == 0);

    /* Break and output into random chunks of 0-12 and 0-16 bytes. */
    in_avail = rand() % 13;
    if (in_avail > enc_size)
        in_avail = enc_size;
    out_avail = rand() % 17;

    ncalls = 0;
    in_avail_used_tot = 0;
    out_avail_used_tot = 0;
    for (;;) {
        ncalls++;
        Uint in_avail_prev = in_avail;
        Uint out_avail_prev = out_avail;
        r = glzwd(decoder_state, enc_buf, dec_buf, &in_avail, &out_avail);
        Uint in_used = in_avail_prev - in_avail;
        enc_buf += in_used;
        Uint out_used = out_avail_prev - out_avail;
        dec_buf += out_used;
        in_avail_used_tot += in_used;
        out_avail_used_tot += out_used;

        if (r == GLZW_OK)
            break;

        if (r == GLZW_NO_INPUT_AVAIL) {
            in_avail = rand() % 13;
            int lefttogo = out_avail_used_tot - in_avail_used_tot;
            if (in_avail >= lefttogo) {
                in_avail = lefttogo;
            }
            continue;
        } else if (r == GLZW_NO_OUTPUT_AVAIL) {
            out_avail = rand() % 17;
            continue;
        } else {
            printf("ERROR: glzwd returned %d\n", r);
            return;
        }
    }
    glzwd_end(decoder_state);
    nticks = clock() - nticks;
    printf("done decoding: glzwd called %d times\n", ncalls);
    int dec_size = out_avail_used_tot;
    printf("decoded size: %d\n", dec_size);
    millisecs = (nticks * 1000L) / (long)(CLOCKS_PER_SEC);
    if ( nticks < 0 )
        printf("***timer error: %ld\n", -nticks);
    else
        printf("decoded in: %ld.%02ld sec\n",
            millisecs/1000, ((millisecs%1000)+5)/10);
    assert(n == dec_size);
    assert(adlersum == adler32(dec_buf_init, dec_size));
#if 1
    for (i = 0; i < dec_size; i++)
        assert(p_init[i] == dec_buf_init[i]);
#endif
}

void try_decode(int opts, int nbits, char *outfile, Uint n, Byte *p)
{
    int r;
    Uint dec_size = n * 2;
    Byte *buf = (Byte *)malloc(dec_size);
    assert(buf);
    Uint lzw_min_code_size = nbits < 2 ? 2 : nbits;
    void *decoder_state;
    r = glzwd_init(&decoder_state, lzw_min_code_size);
    assert(r == 0);
    Uint in_avail = n;
    FILE *fp = fopen(outfile, "wb");
    if (!fp) {
        printf("can't open file %s\n", outfile);
        exit(1);
    }
    for (;;) {
        Uint out_avail = dec_size;
        Uint in_avail_prev = in_avail;
        Uint out_avail_prev = out_avail;
        r = glzwd(decoder_state, p, buf, &in_avail, &out_avail);
        Uint out_sz = out_avail_prev - out_avail;
        size_t k = fwrite(buf, 1, out_sz, fp);
        assert(k == out_sz);
        p += in_avail_prev - in_avail;
        if (r == GLZW_OK) {
#if 0
            printf("in_avail: %d\n", in_avail);
#endif
            assert(in_avail == 0);
            break;
        } else if (r == GLZW_NO_OUTPUT_AVAIL) {
            assert(in_avail);
        } else {
            printf("ERROR: glzwd returned %d\n", r);
            return;
        }
    }
    glzwd_end(decoder_state);
    fclose(fp);
}

void usage(char **msg)
{
    char **p;
    for (p = msg; *p; p++)
        printf("%s\n", *p);
    exit(1);
}

void runlzw(int opts, int nrandoms, int nbits,
                                char *infile, char *outfile, char *dumpfile)
{
    int i, n;
    Byte *p = NULL;
#if 0
    printf("opts: %08x nrandoms: %d nbits: %d\n", opts, nrandoms, nbits);
    printf("infile: %s outfile: %s dumpfile: %s\n", infile, outfile, dumpfile);
#endif
    if (opts & OPT_RANDOM && opts & OPT_INFILE) {
        printf("Can't have both -n and -f options.\n");
        exit(1);
    }
    if (opts & OPT_INFILE) {
        FILE *fp = fopen(infile, "rb");
        if (!fp) {
            printf("can't open file %s\n", infile);
            exit(1);
        }
        printf("File: %s\n", infile);
        fseek(fp, 0L, SEEK_END);
        n = ftell(fp);
        rewind(fp);
        p = (Byte *)malloc((size_t)n);
        assert(p);
        size_t k = fread(p, 1, n, fp);
        assert(k == n);
        printf("%d bytes\n", n);
        fclose(fp);
    } else if (opts & OPT_RANDOM) {
        n = nrandoms;
        p = (Byte *)malloc(n);
        assert(p);
        for (i = 0; i < n; i++)
            p[i] = rand() & ((1 << nbits) - 1);
    } else {
        printf("Must have either -n or -f option.\n");
        exit(1);
    }
#if 1
    if (opts & OPT_DUMPINPUT) {
        FILE *fp = fopen(dumpfile, "wb");
        if (!fp) {
            printf("can't open file %s\n", dumpfile);
            exit(1);
        }
        size_t k = fwrite(p, 1, n, fp);
        assert(k == n);
        fclose(fp);
    }
#endif
    if (opts & OPT_DECODE) {
        try_decode(opts, nbits, outfile, n, p);
        return;
    }
    if (opts & OPT_RANDOM_CHUNKS) {
        run_random_chunks(opts, nbits, outfile, n, p);
    } else {
        run_single_chunk(opts, nbits, outfile, n, p);
    }
}

int main(int argc, char **argv)
{

    int opts = 0;
    int c;
    opterr = 0;

    int nrandoms = 0, nbits = 8;
    char *infile = NULL, *outfile = NULL, *dumpfile = NULL;
    char *str_end;

    while ((c = getopt(argc, argv, "hn:f:o:x:b:edrEgPD")) != -1) {
        switch (c) {
            case 'h':
                usage(usage_msg);
            case 'n':
                opts |= OPT_RANDOM;
                nrandoms = strtoul(optarg, &str_end, 0);
                if (*str_end) {
                    printf("bad -n arg: %s\n", optarg);
                    usage(usage_msg);
                }
                break;
            case 'f':
                opts |= OPT_INFILE;
                infile = optarg;
                break;
            case 'o':
                opts |= OPT_OUTFILE;
                outfile = optarg;
                break;
            case 'x':
                opts |= OPT_DUMPINPUT;
                dumpfile = optarg;
                break;
            case 'b':
                opts |= OPT_NBITS;
                nbits = strtoul(optarg, &str_end, 0);
                if (*str_end || nbits < 1 || nbits > 8) {
                    printf("bad -b arg: %s\n", optarg);
                    usage(usage_msg);
                }
                break;
            case 'e':
                opts |= OPT_ENCODE;
                break;
            case 'd':
                opts |= OPT_DECODE;
                break;
            case 'r':
                opts |= OPT_RANDOM_CHUNKS;
                break;
            case 'E':
                opts |= OPT_ONLY_ENCODE;
                break;
            case 'g':
                opts |= OPT_ONLY_ENCODE;
                break;
            case 'P':
                opts |= OPT_PRINT_ENCODED_CODES;
                print_enc_codes = 1;
                break;
            case 'D':
                opts |= OPT_PRINT_DECODED_CODES;
                print_dec_codes = 1;
                break;
            default:
                abort();
        }
    }
    runlzw(opts, nrandoms, nbits, infile, outfile, dumpfile);
    return 0;
}
