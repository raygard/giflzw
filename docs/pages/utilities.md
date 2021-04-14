---
title: Utilities
layout: page
nav_order: 3
---

# Utilities

## runlzw

[`runlzw`](https://github.com/raygard/test3/blob/main/utilities/runlzw.c) is a program to test and exercise the giflzw library. It may not have the most well-designed command structure. You can use it to encode or decode a file or a string of random bytes.

Here is the usage screen:

```c
    rr -n number or -f infile
    -o outfile
    -x dumpfilename dump random numbers or possibly masked input file
    -b nbits (1-8) will be masked on random numbers or input file

    -e encode infile to outfile
    -d decode infile to outfile
    -r encode and/or decode random chunks

    -E only encode
    -g only encode
    -P print encoded codes
    -D print decoded codes
    -h print usage message
```

All filenames are specified after option flags. The `-n number` and `-f infile` options are mutually exclusive, and one or the other is required. The program will normally encode and decode the file or given number of random bytes and verify that the decoded output matches the input using the `adler32` checksum.

If you specify a `-o outfile` you must also specify `-e` or `-d` to indicate whether to encode or decode the input file. You can also encode a random stream to the output file with the `-n number` option.

Use `-r` to make the program encode and decode random segments of the data incrementally. The results should be the same as if the `-r` were not specified, but the program will run a bit slower due to the extra buffering and calls to the encoding/decoding routines.

## dumpgif

[`dumpgif`](https://github.com/raygard/test3/blob/main/utilities/dumpgif.c) is a program to dump information about a GIF image file in a somewhat readable form. It can optionally dump the LZW data stream, decoded pixel bytes, or a crude BMP file that corresponds to the GIF image. 

The BMP file output is currently for debugging use only. The program makes no attempt to put the BMP pixel rows in correct order. If the GIF is non-interlaced, the BMP will be inverted, because BMP files are ordered with the bottom row of pixels first and the top row last. If the GIF is interlaced, the BMP will look pretty strange. This was only intended as a rough test to see if the decoding was looking correct.

No attempt is made to do anything sensible with an animated GIF.

The information in the dump is written in the order that the GIF blocks are encountered. Unless you are very conversant with the GIF specification, you will probably want to have the GIF spec close at hand when you look at the dump output.

Here is the usage screen:

```c
    dumpgif file.gif
    -p filename -- write pixel data
    -z filename -- write lzw data (de-blocked)
    -b filename -- write a bmp file
    -g     hex dump global color table (GCT)
    -l     hex dump all local color tables (LCTs)
```

All filenames are specified after option flags.
