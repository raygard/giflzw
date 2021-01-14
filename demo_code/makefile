CC=gcc
COPTS=-Wall -Wc++-compat -std=c90

all: lzwe_basic lzwe_clear lzwe_vwc lzwd_basic lzwd_clear lzwd_vwc

lzwe_basic: lzwe_basic.c
	$(CC) $(COPTS) lzwe_basic.c -o lzwe_basic

lzwe_clear: lzwe_clear.c
	$(CC) $(COPTS) lzwe_clear.c -o lzwe_clear

lzwe_vwc: lzwe_vwc.c
	$(CC) $(COPTS) lzwe_vwc.c -o lzwe_vwc

lzwd_basic: lzwd_basic.c
	$(CC) $(COPTS) lzwd_basic.c -o lzwd_basic

lzwd_clear: lzwd_clear.c
	$(CC) $(COPTS) lzwd_clear.c -o lzwd_clear

lzwd_vwc: lzwd_vwc.c
	$(CC) $(COPTS) lzwd_vwc.c -o lzwd_vwc
