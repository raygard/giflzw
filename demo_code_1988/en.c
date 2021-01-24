/* compress LZW files
** Copyright 1988 Ray Gardner
** All Rights Reserved
*/
#include	"stdio.h"

#define	MAXBYTE		255
#define	EOFCD			(MAXBYTE + 1)
#define	NXTCD			(EOFCD + 1)
#define	CODEBITS		12
#define	CODELIM		(1 << CODEBITS)		/* 2 ^ 12 == 4096 */

#define	NOCODE		(-1)

#define	TBLSIZE		5003

int head_tbl[TBLSIZE];
unsigned char tail_tbl[TBLSIZE];
int code_tbl[TBLSIZE];

int nextcode = NXTCD;

FILE *inf, *outf;

int codebits = 12;
int codebuf = 0;
int bufbits = 8;

putcode (code)
int code;
{
	int k, nbits;

	for ( k = codebits; k; k -= nbits ) {
		nbits = bufbits < k ? bufbits : k;
		codebuf |= (code & ~(-1 << nbits)) << (8 - bufbits);
		code >>= nbits;
		bufbits -= nbits;
		if ( bufbits == 0 ) {
			putc(codebuf, outf);
			codebuf = 0;
			bufbits = 8;
		}
	}
}

puteof ()
{
	putcode(EOFCD);
	if ( bufbits )
		putc(codebuf, outf);
}

int probe;

int codeval (head, tail)
int head, tail;
{
	probe = (tail << 1) ^ head;
	while ( head_tbl[probe] != head || tail_tbl[probe] != tail ) {
		if ( head_tbl[probe] == NOCODE )
			return NOCODE;
		if ( (probe -= (tail|256)) < 0 )
			probe += TBLSIZE;
	}
	return code_tbl[probe];
}

setval (head, tail, code)
int head, tail, code;
{
	head_tbl[probe] = head;
	tail_tbl[probe] = tail;
	code_tbl[probe] = code;
}
		
encode ()
{
	int i, head, tail, code;

	for ( i = 0; i < TBLSIZE; i++ )
		head_tbl[i] = NOCODE;
	head = getc(inf);
	if ( head != EOF ) {
		while ( (tail = getc(inf)) != EOF ) {
			if ( (code = codeval(head, tail)) != NOCODE ) {
				head = code;
			} else {
				putcode(head);
				if ( nextcode < CODELIM )
					setval(head, tail, nextcode++);
				head = tail;
			}
		}
		putcode(head);
	}
	puteof();
}

main (argc, argv)
int argc;
char **argv;
{
	if ( argc < 3 ) {
		fprintf(stderr, "Usage: encode origfile compressedfile\n");
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
	encode();
}
