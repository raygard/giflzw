CC=gcc
COPTS=-Wall -Wc++-compat -std=c90 -I ../src
LIB=../src/glzwe.c ../src/glzwd.c
HDRS=../src/glzwe.h ../src/glzwd.h

all: encode decode

encode: encode.c $(LIB) $(HDRS)
	$(CC) $(COPTS) $(LIB) encode.c -o encode

decode: decode.c $(LIB) $(HDRS)
	$(CC) $(COPTS) $(LIB) decode.c -o decode
