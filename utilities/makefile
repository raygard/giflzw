CC=gcc
COPTS=-Wall -Wc++-compat -std=c99 -I ../src

LIB=../src/glzwe.c ../src/glzwd.c
HDRS=../src/glzwe.h ../src/glzwd.h

all: dumpgif.exe runlzw.exe

SRC=dumpgif.c xdump.c writebmp.c crc32_fast.c
dumpgif.exe: $(SRC) xdump.h writebmp.h crc32.h $(LIB) $(HDRS)
	$(CC) $(COPTS) $(SRC) $(LIB) -o dumpgif

COPTS2=-Wall -Wc++-compat -std=c90 -I ../src

SRC2=runlzw.c xdump.c adler32.c
runlzw.exe: $(SRC2) xdump.h adler32.h $(LIB) $(HDRS)
	$(CC) $(COPTS2) $(SRC2) $(LIB) -DTESTDEV -o runlzw
