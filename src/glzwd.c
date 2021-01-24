/* glzwd.c -- GIF LZW incremental decoder
 * Copyright 2021 Raymond D. Gardner
 */

#ifdef TESTDEV
#include <stdio.h>
#endif
/* stdlib.h for calloc()/free(). */
#include <stdlib.h>

#include "glzwd.h"

#ifdef TESTDEV
int print_dec_codes;                            /* Temp for dev. */
#endif

/* values of entry_state */
enum { LZW_INITIAL, LZW_FINISHED,
    LZW_TRY_IN, LZW_TRY_OUT1, LZW_TRY_OUT2 };

/* values of control_state */
enum { ST_INITIAL, ST_NORMAL };

static void glzwd_reset(Glzwd_state *st)
{
    st->next_code = st->end_code + 1;
    st->max_code = 2 * st->clear_code - 1;
    st->code_width = st->lzw_min_code_width + 1;
    st->control_state = ST_INITIAL;
}

int glzwd_init(void **pstate, const GLZWUint lzw_min_code_width)
{
    Glzwd_state *st = (Glzwd_state *)calloc(1, sizeof(Glzwd_state));
    *pstate = st;
    if (!st)
        return GLZW_OUT_OF_MEMORY;
    st->lzw_min_code_width = lzw_min_code_width;
    st->clear_code = 1 << st->lzw_min_code_width;
    st->end_code = st->clear_code + 1;
    st->stack_ptr = st->stack;
    glzwd_reset(st);
    st->bits_in_buf = 0;
    return GLZW_OK;
}

int glzwd(void *state, const GLZWByte *in_ptr, GLZWByte *out_ptr,
        GLZWUint *in_avail, GLZWUint *out_avail)
{
    Glzwd_state *st = (Glzwd_state *)state;
    switch (st->resume_state) {

    case LZW_INITIAL:
get_code:
        st->code_bits_needed = st->code_width;
        st->code = 0;
        while (st->code_bits_needed) {
            if (!st->bits_in_buf) {

    case LZW_TRY_IN:
                if (!*in_avail) {
                    st->resume_state = LZW_TRY_IN;
                    return GLZW_NO_INPUT_AVAIL;
                }
                st->code_buffer = *in_ptr++;
                (*in_avail)--;
                st->bits_in_buf = 8;
            }
            GLZWUint n = st->bits_in_buf < st->code_bits_needed ?
                                    st->bits_in_buf : st->code_bits_needed;
            st->code |= (st->code_buffer & ((1 << n) - 1)) <<
                                    (st->code_width - st->code_bits_needed);
            st->code_buffer >>= n;
            st->bits_in_buf -= n;
            st->code_bits_needed -= n;
        }

        /* Got a code. */
#ifdef TESTDEV
if (print_dec_codes) printf("dec code: %d\n", st->code);
#endif
        if (st->code == st->end_code) {
            st->resume_state = LZW_FINISHED;
            return GLZW_OK;
        }
        if (st->code == st->clear_code) {
            glzwd_reset(st);
            goto get_code;
        }
        if (st->control_state == ST_INITIAL) {
            st->first_byte = st->prev_code = st->code;
            if (st->code > st->end_code)
                return GLZW_INVALID_DATA;

    case LZW_TRY_OUT1:
            if (!*out_avail) {
                st->resume_state = LZW_TRY_OUT1;
                return GLZW_NO_OUTPUT_AVAIL;
            }
            *out_ptr++ = st->first_byte;
            (*out_avail)--;
            st->control_state = ST_NORMAL;
            goto get_code;
        }
        st->in_code = st->code;
        /* Handle the KwKwK case. */
        if (st->code >= st->next_code) {
            if (st->code != st->next_code)
                return GLZW_INVALID_DATA;
            *st->stack_ptr++ = st->first_byte;
            st->code = st->prev_code;
        }
        /* "Unwind" code's string to stack */
        while (st->code >= st->clear_code) {
            /* heads are packed to left of tails in codes */
            /* tails */
            *st->stack_ptr++ = st->codes[st->code] & 0xFF;
            /* heads */
            st->code = (st->codes[st->code] >> 8) & 0xFFF;
        }
        *st->stack_ptr++ = st->first_byte = st->code;

        /* TODO: Here we may be able to run faster by checking for
         * and only moving as many bytes as we have room for.
         */
        do {

    case LZW_TRY_OUT2:
            if (!*out_avail) {
                st->resume_state = LZW_TRY_OUT2;
                return GLZW_NO_OUTPUT_AVAIL;
            }
            *out_ptr++ = *--st->stack_ptr;
            (*out_avail)--;
        } while (st->stack_ptr > st->stack);

        if (st->next_code < CODE_LIMIT) {
            /* heads are packed to left of tails in codes */
            st->codes[st->next_code++] =
                                        (st->prev_code << 8) | st->code;
            if (st->next_code > st->max_code && st->next_code < CODE_LIMIT) {
                st->max_code = st->max_code * 2 + 1;
                st->code_width++;
            }
        }
        st->prev_code = st->in_code;
        goto get_code;

    case LZW_FINISHED:
        return GLZW_OK;

    default:
        return GLZW_INTERNAL_ERROR;
    }
}

void glzwd_end(void *state)
{
    free(state);
}
