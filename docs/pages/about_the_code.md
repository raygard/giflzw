---
title: About the code
layout: page
nav_order: 5
---

## Why and how giflzw was developed

The Python Imaging Library was written by Fredrik Lundh in the late 1990s, at a time when the LZW patent owned by Unisys was in force and Unisys was giving mixed, changing, and ambiguous threats regarding licensing and enforcement. You can read all about it if you search the Web; I won't rehash that here. But I assume Fredrik was intending to avoid the patent (never mentioned in the source code as far as I can see) by initially writing uncompressed GIF files. I assume that meant writing every pixel byte as a 9-bit field, thus making the GIF about 12.5% larger than an uncompressed BMP file. He later developed a technique to write GIF data in a sort of run-length-encoded form that is compatible with GIF decoders but avoided use of the LZW algorithm.

The LZW algorithm patent lapsed in 2004, and some other GIF programs have since then incorporated or re-incorporated LZW compression code. It has been over 15 years and I thought it was time for PIL to get with the program too.

I wrote a [couple of](https://github.com/raygard/test3/blob/main/demo_code_1988/en.c) [demo LZW](https://github.com/raygard/test3/blob/main/demo_code_1988/de.c) programs a few years ago (pre-ANSI C, but compiles with warnings and works). I recently wrote a [crude program](https://github.com/raygard/test3/blob/main/utilities/dumpgif.c) to dump GIF files. After extracting the LZW stream, I used the old demo decoding program as a starting point to get a working GIF decoder. After that, I developed a GIF encoder that created streams I could decode with the new decoder. Then I worked out the current giflzw library.

## Code structure

The main reason for writing this code was to create a GIF LZW encoder suitable for incorporating into the [Python Imaging Library](https://github.com/python-pillow/Pillow) (PIL or Pillow). This imposed certain requirements on the code that led to a program structure that you may find unusual.

Why incremental? The PIL GifEncode.c module accepts bytes to be encoded in chunks, typically of about 64 kB at a time. GIF image data streams are packed into "sub-blocks" of up to 255 bytes each, preceded by a byte containing a byte count. So being able to accept and emit arbitrary blocks of data on each call is a needed feature.

The interface was somewhat inspired by the use of `avail_in` and `avail_out` in the `z_stream` structure of *zlib*.

Before cavilling about the abuse of the switch/case statement or the use of goto statements, I invite you to have a look at 
Simon Tatham's (author of the PuTTY SSH/Telnet client) [piece on coroutines](https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html).

Consider also that in his classic, *Structured Programming with goto Statements* (ACM Computing Surveys, December 1974), Donald Knuth suggests that goto statements can be used to implement coroutines in a language that lacks them as a built-in structure. ([See here](http://www.kohala.com/start/papers.others/knuth.dec74.html#coroutines).)

These encoding and decoding routines are incremental. They need to be able to proceed with encoding or decoding a piece at a time. This requires a kind of semicoroutine, the ability to return and then continue from the point of return the next time it is called.

The switch/case structure is used in a fashion similar to Duff's Device, to facilitate the semicoroutine linkage when the code is called back after having returned when input data or output buffer space is exhausted. Also, in the encoder, the `put_code` "routine" needs to be called from multiple places. It would be nice if I could have made it a separate function, as it is in the demo programs described in "LZW: How it works", but that would make the semicoroutine linkage much more difficult. So instead, I inlined the `put_code` logic and used a state variable, a switch at the end of `put_code`, and goto statements to simulate returns from the `put_code` logic.

If anyone can show me how to accomplish the goals of this library with a better or simpler structure, without loss of (or better, with an improvement of) efficiency, please let me know.

## Performance

In my tests of the library, performance seems most sensitive to the choices used in the hash table: the size of the table, the hash function, and the function used to determine the reprobe interval. I created a 300MB test file by extracting the pixel data (decoded image data) from three sources: a GIF made from an image of a painting, several GIF comic strips, and a GIF made from a photo of colorful medieval dancers. Each source was repeated until it reached 100,000,000 bytes, then the three were concatenated into a single file. I then experimented encoding this file with various table sizes, hash functions, and reprobing strategies. I tried to balance the "randomness" of the hash function against its complexity (and thus how long it takes to compute it). It seemed that getting the fewest reprobes was also critical to performance, and some reprobe strategies were much worse than others.

If anyone can significantly improve the hash table performance, please let me know how.
