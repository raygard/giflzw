CC=gcc
COPTS=-Wall -Wc++-compat -std=c99 -I ../src

LIB=../src/glzwe.c ../src/glzwd.c
HDRS=../src/glzwe.h ../src/glzwd.h

all: dumpgif runlzw

SRC=dumpgif.c xdump.c writebmp.c
dumpgif: $(SRC) xdump.h writebmp.h $(LIB) $(HDRS)
	$(CC) $(COPTS) $(SRC) $(LIB) -o dumpgif

COPTS2=-Wall -Wc++-compat -std=c90 -I ../src

SRC2=runlzw.c xdump.c adler32.c
runlzw: $(SRC2) xdump.h adler32.h $(LIB) $(HDRS)
	$(CC) $(COPTS2) $(SRC2) $(LIB) -DTESTDEV -o runlzw
