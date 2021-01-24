/* glzwd.h -- GIF LZW incremental decoder interface
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
#define GLZW_INVALID_DATA       5

#define CODE_LIMIT              4096
#define STACK_SIZE              4096

typedef struct Glzwd_state {
    GLZWUint control_state;
    GLZWUint resume_state;
    GLZWUint lzw_min_code_width;
    GLZWUint clear_code, end_code, next_code, max_code;
    GLZWUint code_width, code_bits_needed, bits_in_buf;
    GLZWUint code_buffer, first_byte;
    GLZWUint code, in_code, prev_code;
    GLZWUint codes[CODE_LIMIT];
    GLZWByte stack[STACK_SIZE], *stack_ptr;
} Glzwd_state;

int glzwd_init(void **pstate, const GLZWUint lzw_min_code_width);

int glzwd(void *state, const GLZWByte *in_ptr, GLZWByte *out_ptr,
        GLZWUint *in_avail, GLZWUint *out_avail);

void glzwd_end(void *state);
