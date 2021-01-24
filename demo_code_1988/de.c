/* decompress LZW files
** Copyright 1988 Ray Gardner
** All Rights Reserved
*/
#include "stdio.h"

#define	MAXBYTE		255
#define	EOFCD			(MAXBYTE + 1)
#define	NXTCD			(EOFCD + 1)
#define	CODEBITS		12
#define	CODELIM		(1 << CODEBITS)		/* 2 ^ 12 == 4096 */

#define	CODEMASK		(CODELIM-1)

#define	TBLSIZE		CODELIM

int head_tbl[TBLSIZE];
char tail_tbl[TBLSIZE];

int nextcode = NXTCD;

FILE *inf, *outf;

int codebits = 12;
int codebuf = 0;
int bufbits = 0;

int getcode ()
{
	int k, nbits;
	int code = 0;
	for ( k = codebits; k; k -= nbits ) {
		if ( bufbits == 0 ) {
			codebuf = getc(inf);
			bufbits = 8;
		}
		nbits = bufbits < k ? bufbits : k;
		code |= (codebuf & ~(-1 << nbits)) << (codebits - k);
		codebuf >>= nbits;
		bufbits -= nbits;
	}
	return code;
}

#define	STACKSIZE		CODELIM

char stack[STACKSIZE], *stackptr, *stacklim;

decode ()
{
	int i, code, incode, prevcode, firstchar;

	for ( i = 0; i <= MAXBYTE; i++ )
		tail_tbl[i] = i;

	stackptr = stacklim = &stack[STACKSIZE];
	firstchar = prevcode = getcode();
	if ( prevcode != EOFCD ) {
		putc(firstchar, outf);
		while ( (code = getcode()) != EOFCD ) {
			incode = code;
			if ( code >= nextcode ) {
				*--stackptr = firstchar;
				code = prevcode;
			}
			while ( code > MAXBYTE ) {
				*--stackptr = tail_tbl[code];
				code = head_tbl[code];
			}
			*--stackptr = firstchar = code;
			do
				putc(*stackptr, outf);
			while ( ++stackptr < stacklim );
			if ( nextcode < CODELIM ) {
				head_tbl[nextcode] = prevcode;
				tail_tbl[nextcode++] = code;
			}
			prevcode = incode;
		}
	}
}

main (argc, argv)
int argc;
char **argv;
{

	if ( argc < 3 ) {
		fprintf(stderr, "Usage: decode compressedfile origfile\n");
		exit(0);
	}
	inf = fopen(argv[1], "rb");
	if ( inf == NULL ) {
		fprintf(stderr, "can't open input file %s\n", argv[1]);
		exit(1);
	}
	outf = fopen(argv[2], "wb");
	if ( outf == NULL ) {
		fprintf(stderr, "can't open output file %s\n", argv[2]);
		exit(1);
	}
	decode();
}
