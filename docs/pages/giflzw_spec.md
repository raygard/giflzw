---
title: giflzw specification
layout: page
nav_order: 2
---

[//]: # superscript n ⁿ is U+207F

# giflzw

[//]: # s### TODO: fix so it works if END code is missing?

giflzw is a small C library for _incrementally_ encoding and decoding GIF image data streams using the LZW algorithm. I usually refer to encoding and decoding rather than compression and decompression. The [GIF 89a specification](https://www.w3.org/Graphics/GIF/spec-gif89a.txt) (and [older 87a specification](https://www.w3.org/Graphics/GIF/spec-gif87.txt)) frequently use the same terms.

The giflzw library is general enough it may be used for encoding and decoding data other than GIF image data, but there are better alternatives in probably every case.


---

## Common definitions and declarations

The definitions needed to use the library are:

```c
#define GLZW_OK                 0
#define GLZW_NO_INPUT_AVAIL     1
#define GLZW_NO_OUTPUT_AVAIL    2
#define GLZW_OUT_OF_MEMORY      3
#define GLZW_INTERNAL_ERROR     4
#define GLZW_INVALID_DATA       5
```

`GLZWUint` is a typedef for an unsigned integer of at least 32 bits. `GLZWByte` is `unsigned char`. These types are used in the library to avoid collisions with names in programs that include the `<glzwe.h>` and `<glzwd.h>` header files.

In the examples here, I use `Uint` and `Byte` for these types.

---

## Encoding (compression)

### Header file for the encoding routines

```c
#include <glzwe.h>
```

---

### Initialization

```c
int glzwe_init(void **pstate, const GLZWUint lzw_min_code_width);
```

Call once to initialize the encoder for encoding one stream of bytes. The function will allocate and initialize a structure and return a pointer to it via `pstate`. The function also requires an integer that is described as "LZW Minimum Code Size" in section 22 of the [GIF 89a specification](https://www.w3.org/Graphics/GIF/spec-gif89a.txt).

`pstate` will be used to return a pointer to the encoder's internal state. This is "opaque"; the user should not depend on what is inside the library's state structure.

`lzw_min_code_width` is an integer, at least 2 and at most 8, representing the width in bits of the data to be encoded. If whole bytes are to be encoded (as for a 256-color palette), use 8. For smaller palettes (color tables) of size 2ⁿ (2 <= n < 8), use the corresponding n. For 2-color palettes, use 2 (see the specification for details). Note that the data must be unpacked, that is, each byte in the stream contains a single value referring to a color table entry.

Returns: `GLZW_OUT_OF_MEMORY` if the structure cannot be allocated, or `GLZW_OK` otherwise.

Example: To begin encoding a stream of bytes, use:

```c
void *state;
int ret = glzwe_init(&state, 8);
```

---

### Encoding

```c
int glzwe(void *state, const GLZWByte *in_ptr, GLZWByte *out_ptr,
        GLZWUint *in_avail, GLZWUint *out_avail,
        GLZWUint end_of_data);
```

Call as many times as needed to encode a stream of bytes. It is the caller's responsibility to ensure that bytes have a value less than 2<sup>lzw_min_code_width</sup>.

`state` is a pointer to the structure initialized by `glzwe_init()`.<br/>
`in_ptr` is a pointer to a buffer containing bytes to be encoded.<br/>
`out_ptr` is a pointer to an output buffer that will contain the resulting codes.<br/>
`in_avail` is a pointer to a count of bytes in the input buffer.<br/>
`out_avail` is a pointer to a count of bytes left in the output buffer.<br/>
`end_of_data` is a flag that is zero if there is more data to be encoded than is present in the input buffer, and non-zero when the input buffer contains the last bytes in the stream to be encoded. In the case where `end_of_data` is non-zero, the input buffer may be empty, i.e. `in_avail` points to a variable containing 0.

This function supports incremental encoding, i.e. a chunk of input or output at a time. The function will encode the data at the location referenced by `in_ptr`, storing output codes at the location referenced by `out_ptr`. As bytes are consumed or emitted, `in_avail` and `out_avail` are decremented. The function returns when either one reaches zero and it attempts to consume or produce more data.

The caller must keep track of how far `in_avail` and `out_avail` are decremented, and must also ensure the `in_ptr` and `out_ptr` are updated as appropriate before the next call.

Returns: <br/>
`GLZW_NO_INPUT_AVAIL` when `in_avail` reaches zero and `end_of_data` is not set. Caller must call again with at least one byte of input available.<br/>
`GLZW_NO_OUTPUT_AVAIL` when `out_avail` reaches zero. Caller must call again with at least one byte of output space available.<br/>
`GLZW_OK` after the last byte of output has been stored.<br/>
`GLZW_INTERNAL_ERROR` can only occur if the state structure is tampered with.

Example: To encode a stream of bytes, you could use:

{: .lh-tight }
```c
    Uint incnt, incnt0, outcnt;
    enum {inbufsize = 30, outbufsize = 10};
    Byte inbuf[inbufsize], outbuf[outbufsize];
    // ... open input and output to infp, outfp;
    void *state;
    int r = glzwe_init(&state, 8);
    Byte *p = inbuf;
    incnt = 0;
    int end_of_data = 0;
    do {
        outcnt = outbufsize;
        incnt0 = incnt;
        r = glzwe(state, p, outbuf, &incnt, &outcnt, end_of_data);
        fwrite(outbuf, 1, outbufsize - outcnt, outfp);
        if (r == GLZW_NO_INPUT_AVAIL) {
            p = inbuf;
            incnt = fread(p, 1, inbufsize, infp);
            end_of_data = incnt < inbufsize;
        } else if (r == GLZW_NO_OUTPUT_AVAIL) {
            p += incnt0 - incnt;
        } else if (r != GLZW_OK) {
            printf("ERROR r:%d\n", r);
            abort();
        }
    } while (r != GLZW_OK);
    // close files; call glzwe_end(state);
```

(A complete version of this program is in the [`examples` code directory](https://github.com/raygard/test3/blob/main/examples/encode.c).)

Notes on this example: The input buffer is intially empty. The first call to `glzwe()` will return `GLZW_NO_INPUT_AVAIL` and then the input will be read. After each call to `glzwe()`, any output produced will be written. Then, if the input buffer is exhausted, more data will be read. The `end_of_data` flag is set when all input is read. If the output buffer is full, the input buffer pointer is adjusted to pick up the rest of the input on the next call to `glzwe()`. This program will work for any sizes of input and output buffers.

---

### Finishing

```c
void glzwe_end(void *state);
```

Release the memory allocated by `glzwe_init()` for the state structure.

---

## Decoding (decompression)

---

### Header file for the decoding routines

```c

#include <glzwd.h>
```

---

### Initialization

```c
int glzwd_init(void **pstate, const GLZWUint lzw_min_code_width);
```

Call once to initialize the decoder for decoding one stream of bytes. The function will allocate and initialize a structure and return a pointer to it via `pstate`. The function also requires an integer that is described as "LZW Minimum Code Size" in section 22 of the [GIF 89a specification](https://www.w3.org/Graphics/GIF/spec-gif89a.txt).

`pstate` will be used to return a pointer to the decoder's internal state. This is "opaque"; the user should not depend on what is inside the library's state structure.

`lzw_min_code_width` is an integer, at least 2 and at most 8, representing the width in bits of the data to be decoded. It must match the value used to encode the data, as described above for `glzwe_init()`.

Returns: `GLZW_OUT_OF_MEMORY` if the structure cannot be allocated, or `GLZW_OK` otherwise.

Example: To begin decoding a stream of bytes, use:

```c
void *state;
int ret = glzwd_init(&state, 8);
```

### Decoding

```c
int glzwd(void *state, const GLZWByte *in_ptr, GLZWByte *out_ptr,
        GLZWUint *in_avail, GLZWUint *out_avail);
```

Call as many times as needed to decode a stream of bytes.

`state` is a pointer to the structure initialized by `glzwd_init()`.<br/>
`in_ptr` is a pointer to a buffer containing bytes to be decoded.<br/>
`out_ptr` is a pointer to an output buffer that will contain the resulting bytes.<br/>
`in_avail` is a pointer to a count of bytes in the input buffer.<br/>
`out_avail` is a pointer to a count of bytes left in the output buffer.<br/>

This function supports incremental decoding, i.e. a chunk of input or output at a time. The function will decode the data at the location referenced by `in_ptr`, storing output bytes at the location referenced by `out_ptr`. As bytes are consumed or emitted, `in_avail` and `out_avail` are decremented. The function returns when either one reaches zero and it attempts to consume or produce more data.

The caller must keep track of how far `in_avail` and `out_avail` are decremented, and must also ensure the `in_ptr` and `out_ptr` are updated as appropriate before the next call.

Returns: <br/>
`GLZW_NO_INPUT_AVAIL` when `in_avail` reaches zero. Caller must call again with at least one byte of input available.<br/>
`GLZW_NO_OUTPUT_AVAIL` when `out_avail` reaches zero. Caller must call again with at least one byte of output space available.<br/>
`GLZW_OK` after the last byte of output has been stored.<br/>
`GLZW_INVALID_DATA` if the input bytes form an invalid stream of LZW codes.<br/>
`GLZW_INTERNAL_ERROR` can only occur if the state structure is tampered with.

Example: To decode a stream of bytes, you could use:

{: .lh-tight }
```c
    Uint incnt, incnt0, outcnt;
    enum {inbufsize = 30, outbufsize = 10};
    Byte inbuf[inbufsize], outbuf[outbufsize];
    // ... open input and output to infp, outfp;
    void *state;
    int r = glzwd_init(&state, 8);
    Byte *p = inbuf;
    incnt = 0;
    do {
        outcnt = outbufsize;
        incnt0 = incnt;
        r = glzwd(state, p, outbuf, &incnt, &outcnt);
        fwrite(outbuf, 1, outbufsize - outcnt, outfp);
        if (r == GLZW_NO_INPUT_AVAIL) {
            p = inbuf;
            incnt = fread(p, 1, inbufsize, infp);
            if (!incnt) {
                printf("ERROR: empty or bad input.\n");
                abort();
            }
        } else if (r == GLZW_NO_OUTPUT_AVAIL) {
            p += incnt0 - incnt;
        } else if (r != GLZW_OK) {
            printf("ERROR r:%d\n", r);
            abort();
        }
    } while (r != GLZW_OK);
    // close files; call glzwd_end(state);
```

(A complete version of this program is in the [`examples` code directory](https://github.com/raygard/test3/blob/main/examples/decode.c).)

Notes on this example: This is similar to the example for encoding. When encoding, the encoder must be notified via the `end_of_data` argument that all input has been supplied. When decoding, this is not needed because a well-formed coded input stream will have an end code. Instead, we observe that if the decoder expects more input (`GLZW_NO_INPUT_AVAIL`) and no more input is available (`incnt` is zero), then the input must be empty or not well-formed.

The above approach will detect if the input file is truncated, but if you also wish to detect if there is extra data after the END code, you could insert this after the `do ... while()`:

{: .lh-tight }
```c
    if (incnt || fread(inbuf, 1, 1, infp)) {
        printf("ERROR: junk after END\n");
        abort();
    }
```

### Finishing

```c
void glzwd_end(void *state);
```

Release the memory allocated by `glzwd_init()` for the state structure.

