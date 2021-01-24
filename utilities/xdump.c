/* xdump.c -- hex and character dump function
 * Copyright 2021 Raymond D. Gardner
 *
 *      Usage: xdump(baddr, nbytes, label)
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "xdump.h"

static int match(char *p, char *q, unsigned n)
{
    while (n--)
        if (*p++ != *q++)
            return 0;
    return 1;
}

static char *ln, *p;

static void f()
{
    int ch;

    ch = *p >> 4 & 15; ln[0] = ch > 9 ? ch + '7' : ch + '0';
    ch = *p++ & 15;  ln[1] = ch > 9 ? ch + '7' : ch + '0';
    ln[2] = ' '; ln += 3;
}

static int last [] = {
    0, 2, 5, 8, 11, 15, 18, 21, 24, 29, 32, 35, 38, 42, 45, 48, 51
    };

#define maxline 85
void xdump(void *baddr_, size_t nbytes, unsigned label)
{
    int k, show, skipped;
    char line[maxline], *baddr0, *pln, *lim;

    char *baddr = (char *)baddr_;
    baddr0 = baddr;
    skipped = 0;
    show = 1;
    while (nbytes > 0) {
        k = nbytes < 16 ? nbytes : 16;
        if (baddr == baddr0 || nbytes <= 16 ||
          (show = ! match(baddr, baddr-16, 16)) != 0) {
            ln = line;
            if ((int)label != -1) {
                sprintf(ln, "%08X  ", label);
                ln += 10;
            }
            *ln++ = skipped ? '*' : ' ';
            pln = ln;
            p = baddr;
            f(); f(); f(); f(); *ln++ = ' ';
            f(); f(); f(); f(); *ln++ = '-'; *ln++ = ' ';
            f(); f(); f(); f(); *ln++ = ' ';
            f(); f(); f(); f();
            memset(pln+last[k], ' ', 56-last[k]);
            pln += 56;
            lim = pln + k;
            for (p = baddr; pln < lim; p++)
                *pln++ = (31 < *p && *p < 127) ? *p : '.';
            *pln++ = '\n';
            *pln = '\0';
            assert(pln < line + maxline);
            fputs(line, stdout);
        }
        skipped = ! show;
        baddr += k;
        nbytes -= k;
        if ((int)label != -1)
            label += k;
    }
}
