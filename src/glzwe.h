/* glzwe.h -- GIF LZW incremental encoder interface
 * Copyright 2021 Raymond D. Gardner
 */

typedef unsigned char GLZWByte;
typedef unsigned int GLZWUint;

/* Return values */
#define GLZW_OK                 0
#define GLZW_NO_INPUT_AVAIL     1
#define GLZW_NO_OUTPUT_AVAIL    2
#define GLZW_OUT_OF_MEMORY      3
#define GLZW_INTERNAL_ERROR     4

/* Table load factors: with max load of 3838 (=4096-256-2), these table sizes
 * will give these load factors (lower factor means fewer reprobes).  Primes
 * ensure any positive secondary hash is relatively prime.  If power of 2, must
 * have good hash function if using mod tablesize (== use lower bits of hash).
 * 4096 .937; 4801 .799; 5101 .752; 6829 .562; 8221 .467; 8192 .469 */

#define FASTER_HASH  1
#if FASTER_HASH
#define TABLE_SIZE  8192
#else
#define TABLE_SIZE  4801
#endif

typedef struct Glzwe_state {
    GLZWUint put_state;
    GLZWUint entry_state;
    GLZWUint lzw_min_code_width;
    GLZWUint clear_code, end_code, next_code, max_code;
    GLZWUint code_width, code_bits_left, buf_bits_left;
    GLZWUint code_buffer;
    GLZWUint head, tail;
    int probe;
    GLZWUint code;
    GLZWUint codes[TABLE_SIZE];
} Glzwe_state;

int glzwe_init(void **pstate, const GLZWUint lzw_min_code_width);

int glzwe(void *state, const GLZWByte *in_ptr, GLZWByte *out_ptr,
        GLZWUint *in_avail, GLZWUint *out_avail,
        GLZWUint end_of_data);

void glzwe_end(void *state);
