---
title: "LZW: How it works"
layout: page
nav_order: 4
---

# LZW: How it works

## Encoding

The LZW algorithm was introduced by Terry Welch in the article "A Technique for High-Performance Data Compression," IEEE Computer, June 1984, pp 8-19.

The LZW encoder converts its input to a series of numeric codes, and the decoder converts the codes back to the original data. I'll explain the encoding first, giving more detail later.

The encoder keeps a table of byte strings and corresponding code values. The codes range from 0 to 4095. The codes from 0 through 255 represent the corresponding one-byte strings. Two special codes, 256 and 257 are reserved and called CLEAR and END respectively. As the encoder reads the data, it assigns additional codes in ascending order to byte strings as they are encountered in the input.

At each step, the encoder finds the longest string in its table that matches the remaining bytes in the input and emits the corresponding code. I call this string the head. It looks at the next byte, which I call the tail. (Welch calls the head and tail the prefix string and the extension character.) It then assigns the next available code to the string formed by appending the tail to the head. The encoder then continues with the tail as the first byte of the new head.

Welch gives the algorithm as:

```
Initialize table to contain single-character strings.
Read first input character → prefix string ω
Step: Read next input character K
    if no such K (input exhausted): code(ω) → output; EXIT
    if ωK exists in string table: ωK → ω; repeat Step.
    else ωK not in string table: code(ω) → output;
                                 ωK  → string table;
                                 K → ω; repeat Step.
```

My rendition is:

{: .lh-tight }
```python
next_code = 258
head = getbyte()
if head != EOF:
    while (tail = getbyte()) != EOF:
        code = lookup_in_table(head, tail)
        if code is found:
            head = code
        else:   # found empty slot
            put_code(head)
            if next_code < 4096:
                insert_in_table(head, tail, next_code)
                next_code += 1
            head = tail
    put_code(head)
```

[Working C code in the `demo_code` directory:](https://github.com/raygard/giflzw/blob/main/demo_code/lzwe_basic.c) This implementation emits (only) 12-bit codes.

Rather than keeping entire strings explicitly in the table, the usual implementation uses a hash table that has a (code, byte) pair as a key, where the code corresponds to the head just found and the byte is the tail. The value of the table entry is the newly assigned code. There is no need to put the single-byte codes in the table, so it only needs to hold new codes from 258 through 4095, for strings of two or more bytes.


## Decoding

The decoder accepts codes and builds a table that corresponds to the table used by the encoder. One complication is that after reading a code and emitting its string, the decoder cannot add another code and string until it finds the first character of the string of the next code. 


{: .lh-tight }
```python
next_code = 258
first_byte = prev_code = get_code()
if prev_code != END:
    assert(prev_code <= 255)    # Data error if code is not a single byte!
    put_byte(first_byte)
    while (code = get_code()) != END:
        in_code = code
        # This returns the first byte of the string.
        first_byte = put_string_for_code(code)
        if next_code < 4096:
            heads[next_code] = prev_code
            tails[next_code] = first_byte
            next_code += 1
        prev_code = in_code
```

So how do we emit the string for the code? If the code is a single byte (0 through 255), that is the string. Otherwise, the heads and tails tables contain the code for the head (prefix) of the string (all bytes except the last) and the code for the tail (last byte), respectively.

So we "unwind" the string starting at the end. This produces the string in reverse order, so we can use a stack to put it back in correct order:

{: .lh-tight }
```python
put_string_for_code(code):
    while code > 255:
        push(tails[code])
        code = heads[code]
    first_byte = code
    push(first_byte)  # Or put_byte(first_byte)
    while stack_not_empty:
        put_byte(pop(stack))
```

But there is another problem, that Welch calls the KωKωK case, where K is a single byte and ω is a possibly empty string. If Kω  is in the table but KωK is not, the encoder will send the code for Kω, then add a new code for KωK to its table, and then encountering KωK it will send that code. The decoder will write the byte string for Kω but will then need the first byte of the string represented by the next code, but will not yet have the code for KωK in its table. This is the only case where the decoder encounters a code not in its table, and the code must be the next available (unassigned) code. But when this happens, the decoder "knows" that the next code must represent KωK and it has K from the string just decoded, so it can add (Kω, K) to its table.

With this complication accounted for, the decoder looks like this:

{: .lh-tight }
```python
next_code = 258
first_byte = prev_code = get_code()
if prev_code != END:
    assert(prev_code <= 255)    # Data error if code is not a single byte.
    put_byte(first_byte)
    while (code = get_code()) != END:
        in_code = code
        # Handle KwKwK case:
        if code >= next_code:
            assert(code == next_code)   # Guard against bad data.
            push(first_byte)
            code = prev_code
        # Unwind code bytes to stack
        while code > 255:
            push(tails[code])
            code = heads[code]
        first_byte = code
        push(first_byte)  # Or put_byte(first_byte)
        # Put bytes from stack
        while stack_not_empty:
            put_byte(pop(stack))
        # Insert new code head and tail to table.
        if next_code < 4096:
            heads[next_code] = prev_code
            tails[next_code] = first_byte
            next_code += 1
        prev_code = in_code
```

[Working C code in the `demo_code` directory:](https://github.com/raygard/giflzw/blob/main/demo_code/lzwd_basic.c)

## Some improvements

The decoder tables `heads` and `tables` are 4096 elements in these examples and the linked demo programs. They can only hold the dynamically-assigned codes from 258 through 4095, so they can be 258 elements smaller, saving a bit of space. The subscripts to them will need to be offset by -258 to accommodate this.

Welch did not address what happens when the table is full and all 4096 codes have been assigned. If the rest of input data has different characteristics than what has been seen by the encoder so far, many of the longer strings in the table will not be encountered and the rate of compression will fall off. One approach taken by some implementations is to clear the full table, send a special CLEAR code to the output, and start over.

### With a CLEAR code

Here is the encoder logic with a CLEAR code sent when the table is full:

{: .lh-tight }
```python
next_code = 258
head = getbyte()
if head != EOF:
    while (tail = getbyte()) != EOF:
        code = lookup_in_table(head, tail)
        if code is found:
            head = code
        else:   # found empty slot
            put_code(head)
            if next_code < 4096:
                insert_in_table(head, tail, next_code)
                next_code += 1
            else:
                put_code(CLEAR)
                next_code = 258
                reset_table_to_initial_state()
            head = tail
    put_code(head)
```

[Working C code in the `demo_code` directory:](https://github.com/raygard/giflzw/blob/main/demo_code/lzwe_clear.c)

The corresponding decoder looks a bit different from the basic version. As explained in the ["Cover Sheet for the GIF89a Specification"](https://w3.org/Graphics/GIF/spec-gif89a.txt):

> There has been confusion about where clear codes can be found in the data stream.  As the specification says, they may appear at anytime.  There is not a requirement to send a clear code when the string table is full.

> [...] when the decoder's table is full, it must not change the table until a clear code is received.  

So the decoder must be able to handle a CLEAR code at any point, and reset the process to its inital state. One way to do this is to use next_code as a state flag. It may be better to use an explicit state variable instead, but this is how I handled it in the demo program. Here is the modified decoder:

{: .lh-tight }
```python
next_code = 0
while (code = get_code()) != END:
    if (code == CLEAR):
        next_code = 0
    elif next_code == 0:
        next_code = 258
        first_byte = prev_code = code
        assert(prev_code <= 255)    # Data error if code is not a single byte.
        put_byte(first_byte)
    else:
        in_code = code
        # Handle KwKwK case:
        if code >= next_code:
            assert(code == next_code)   # Guard against bad data.
            push(first_byte)
            code = prev_code
        # Unwind code bytes to stack
        while code > 255:
            push(tails[code])
            code = heads[code]
        first_byte = code
        push(first_byte)  # Or put_byte(first_byte)
        # Put bytes from stack
        while stack_not_empty:
            put_byte(pop(stack))
        # Insert new code head and tail to table.
        if next_code < 4096:
            heads[next_code] = prev_code
            tails[next_code] = first_byte
            next_code += 1
        prev_code = in_code
```

[Working C code in the `demo_code` directory:](https://github.com/raygard/giflzw/blob/main/demo_code/lzwd_clear.c)

### With CLEAR code, END code, and variable width codes

The GIF specification calls for an END code to be the last code in the data stream. It also saves space by using smaller fields for lower-valued codes. Codes up to 511 may fit in 9 bits, 512 through 1023 in 10 bits, 1024 through 2047 in 11 bits, and 2048 through 4095 in 12 bits.

Here is a encoder for these requirements:

{: .lh-tight }
```python
next_code = 258
max_code = 511
code_width = 9
reset_table_to_initial_state()
put_code(CLEAR)
head = getbyte()
if head != EOF:
    while (tail = getbyte()) != EOF:
        code = lookup_in_table(head, tail)
        if code is found:
            head = code
        else:   # found empty slot
            put_code(head)
            if next_code < 4096:
                insert_in_table(head, tail, next_code)
                if next_code > max_code:
                    max_code = max_code * 2 + 1
                    code_width += 1
                next_code += 1
            else:
                put_code(CLEAR)
                next_code = 258
                max_code = 511
                code_width = 9
                reset_table_to_initial_state()
            head = tail
    put_code(head)
```

[Working C code in the `demo_code` directory:](https://github.com/raygard/giflzw/blob/main/demo_code/lzwe_vwc.c)

And a corresponding decoder:

{: .lh-tight }
```python
next_code = 0
max_code = 511
code_width = 9
while (code = get_code()) != END:
    if (code == CLEAR):
        next_code = 0
        max_code = 511
        code_width = 9
    elif next_code == 0:
        next_code = 258
        first_byte = prev_code = code
        assert(prev_code <= 255)    # Data error if code is not a single byte.
        put_byte(first_byte)
    else:
        in_code = code
        # Handle KwKwK case:
        if code >= next_code:
            assert(code == next_code)   # Guard against bad data.
            push(first_byte)
            code = prev_code
        # Unwind code bytes to stack
        while code > 255:
            push(tails[code])
            code = heads[code]
        first_byte = code
        push(first_byte)  # Or put_byte(first_byte)
        # Put bytes from stack
        while stack_not_empty:
            put_byte(pop(stack))
        # Insert new code head and tail to table.
        if next_code < 4096:
            heads[next_code] = prev_code
            tails[next_code] = first_byte
            next_code += 1
            if next_code > max_code and next_code < 4096:
                max_code = max_code * 2 + 1
                code_width += 1
        prev_code = in_code
```

[Working C code in the `demo_code` directory:](https://github.com/raygard/giflzw/blob/main/demo_code/lzwd_vwc.c)

Welch anticipated using variable-width code fields, but did not suggest clearing the table or using an explicit END code.

## Variations

As mentioned above, the GIF specification states that the CLEAR code does not have to happen when the table is full. Some implementations achieve better compression by delaying the CLEAR code until the compression rate falls below some threshold, determined heuristically somehow. The Unix 'compress' utility used this approach, as did the now-forgotten 'zoo' archiver by Rahul Dhesi. (I supplied fast decompression code for that program.) I experimented with delaying the clear code until the compression rate falls over the last two intervals of _N_ input bytes, for various _N_. I decided the additional compression was not worth the extra complexity. Also, the code ran slower because of the longer hash table searches after the table fills up. I have not examined the logic in 'compress' or 'zoo'; they may have better performance.

In a more general (non-GIF) case, the maximum codes could be larger than 12 bits, using larger tables than those in the versions shown above.

All of the above versions assume we are encoding byte values that range from 0 through 255. In GIF image files, the bytes being encoded are actually indexes into a table of RGB color pixel values. Each pixel is represented in the table by three bytes, one byte each for red, green, and blue. There can be a maximum of 256 entries in that table. But the table can be smaller, any of 2, 4, 8, 16, 32, 64, 128, or 256 values. A 2-color image needs only 2 pixel values.

So the data being encoded may be limited to values up to 1, 3, 7, ... 255, depending on the size of the color table. The specification states that the smaller color tables can be referenced by smaller bit fields in the LZW data. The 2-color version needs to reserve 0 and 1 for pixel indexes, 4 for CLEAR and 5 for END, so it will need at least 3 bits to allow for additional codes to be determined by the encoder. For other table sizes of 2ⁿ (2 <= n <= 8), the code widths may begin with values of n+1. The spec says the data stream begins with a byte value of 2 through 8 that it calls "LZW Minimum Code Size", but this is actually one less than the minimum code size used, so it varies from 2 through 8. It will be 2 for a 2- or 4-pixel color table.


