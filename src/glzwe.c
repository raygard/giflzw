/* glzwd.c -- GIF LZW incremental encoder
 * Copyright 2021 Raymond D. Gardner
 */

#ifdef TESTDEV
#include <stdio.h>
#endif
/* string.h for memset(); stdlib.h for calloc()/free(). */
#include <string.h>
#include <stdlib.h>

#include "glzwe.h"

#ifdef TESTDEV
int nsuccesses, nfails, nreprobes, ninserts;    /* Temp for dev. */
int print_enc_codes;                            /* Temp for dev. */
#endif

#define CODE_LIMIT  4096

/* values of entry_state */
enum { LZW_INITIAL, LZW_TRY_IN1, LZW_TRY_IN2, LZW_TRY_OUT1, LZW_TRY_OUT2,
    LZW_FINISHED };

/* values of control_state */
enum { PUT_HEAD, PUT_INIT_CLEAR, PUT_CLEAR, PUT_LAST_HEAD, PUT_END };

static void glzwe_reset(Glzwe_state *st)
{
    st->next_code = st->end_code + 1;
    st->max_code = 2 * st->clear_code - 1;
    st->code_width = st->lzw_min_code_width + 1;
    memset(st->codes, 0, sizeof(st->codes));
}

int glzwe_init(void **pstate, const GLZWUint lzw_min_code_width)
{
    /*Glzwe_state *st = *pst = (Glzwe_state *)calloc(1, sizeof(Glzwe_state));
     */
    Glzwe_state *st = (Glzwe_state *)calloc(1, sizeof(Glzwe_state));
    *pstate = st;
    if (!st) {
        return GLZW_OUT_OF_MEMORY;
    }
    st->lzw_min_code_width = lzw_min_code_width;
    st->clear_code = 1 << st->lzw_min_code_width;
    st->end_code = st->clear_code + 1;
    glzwe_reset(st);
    st->entry_state = LZW_INITIAL;
    st->buf_bits_left = 8;
    st->code_buffer = 0;
    return GLZW_OK;
}

int glzwe(void *state, const GLZWByte *in_ptr, GLZWByte *out_ptr,
        GLZWUint *in_avail, GLZWUint *out_avail,
        GLZWUint end_of_data)
{
    Glzwe_state *st = (Glzwe_state *)state;
    switch (st->entry_state) {

    case LZW_TRY_IN1:
get_first_byte:
        if (!*in_avail) {
            if (end_of_data) {
                goto end_of_data;
            }
            st->entry_state = LZW_TRY_IN1;
            return GLZW_NO_INPUT_AVAIL;
        }
        st->head = *in_ptr++;
        (*in_avail)--;

    case LZW_TRY_IN2:
encode_loop:
        if (!*in_avail) {
            if (end_of_data) {
                st->code = st->head;
                st->put_state = PUT_LAST_HEAD;
                goto put_code;
            }
            st->entry_state = LZW_TRY_IN2;
            return GLZW_NO_INPUT_AVAIL;
        }
        st->tail = *in_ptr++;
        (*in_avail)--;

        /* Knuth TAOCP vol 3 algorithm D. */
        /* Hashes found experimentally to be pretty good: */
#if FASTER_HASH
        /* This works ONLY with TABLE_SIZE a power of 2: */
        st->probe = ((st->head ^ (st->tail << 6)) * 31) & (TABLE_SIZE - 1);
#else
        st->probe = (st->head * 211 + st->tail * 17) % TABLE_SIZE;
#endif
        while (st->codes[st->probe]) {
            if ((st->codes[st->probe] & 0xFFFFF) ==
                                        ((st->head << 8) | st->tail)) {
#ifdef TESTDEV
nsuccesses++;
#endif
                st->head = st->codes[st->probe] >> 20;
                goto encode_loop;
            } else {
#ifdef TESTDEV
nreprobes++;
#endif
        /* reprobe decrement must be nonzero and relatively prime to table
         * size. These decrements found experimentally to be pretty good. */
#if FASTER_HASH
                if ((st->probe -= ((st->tail << 2) | 1)) < 0) {
                    st->probe += TABLE_SIZE;
                }
#else
                if ((st->probe -= (st->tail + 1)) < 0) {
                    st->probe += TABLE_SIZE;
                }
#endif
            }
        }
        /* Key not found, probe is at empty slot. */
#ifdef TESTDEV
nfails++;
#endif
        st->code = st->head;
        st->put_state = PUT_HEAD;
        goto put_code;
insert_code_or_clear: /* jump here after put_code */
        if (st->next_code < CODE_LIMIT) {
#ifdef TESTDEV
ninserts++;
#endif
            st->codes[st->probe] = (st->next_code << 20) |
                                    (st->head << 8) | st->tail;
            if (st->next_code > st->max_code) {
                st->max_code = st->max_code * 2 + 1;
                st->code_width++;
            }
            st->next_code++;
        } else {
            st->code = st->clear_code;
            st->put_state = PUT_CLEAR;
            goto put_code;
reset_after_clear: /* jump here after put_code */
            glzwe_reset(st);
        }
        st->head = st->tail;
        goto encode_loop;

    case LZW_INITIAL:
        glzwe_reset(st);
        st->code = st->clear_code;
        st->put_state = PUT_INIT_CLEAR;
put_code:

#ifdef TESTDEV
if (print_enc_codes) printf("enc code: %d\n", st->code);
#endif
        st->code_bits_left = st->code_width;
check_buf_bits:
        if (!st->buf_bits_left) {   /* out buffer full */

    case LZW_TRY_OUT1:
            if (!*out_avail) {
                st->entry_state = LZW_TRY_OUT1;
                return GLZW_NO_OUTPUT_AVAIL;
            }
            *out_ptr++ = st->code_buffer;
            (*out_avail)--;
            st->code_buffer = 0;
            st->buf_bits_left = 8;
        }
        /* code bits to pack */
        GLZWUint n = st->buf_bits_left < st->code_bits_left
                        ? st->buf_bits_left : st->code_bits_left;
        st->code_buffer |=
                        (st->code & ((1 << n) - 1)) << (8 - st->buf_bits_left);
        st->code >>= n;
        st->buf_bits_left -= n;
        st->code_bits_left -= n;
        if (st->code_bits_left)
            goto check_buf_bits;
        switch (st->put_state) {
        case PUT_INIT_CLEAR:
            goto get_first_byte;
        case PUT_HEAD:
            goto insert_code_or_clear;
        case PUT_CLEAR:
            goto reset_after_clear;
        case PUT_LAST_HEAD:
            goto end_of_data;
        case PUT_END:
            goto flush_code_buffer;
        default:
            return GLZW_INTERNAL_ERROR;
        }

end_of_data:
        st->code = st->end_code;
        st->put_state = PUT_END;
        goto put_code;
flush_code_buffer: /* jump here after put_code */
        if (st->buf_bits_left < 8) {

    case LZW_TRY_OUT2:
            if (!*out_avail) {
                st->entry_state = LZW_TRY_OUT2;
                return GLZW_NO_OUTPUT_AVAIL;
            }
            *out_ptr++ = st->code_buffer;
            (*out_avail)--;
        }
        st->entry_state = LZW_FINISHED;
        return GLZW_OK;

    case LZW_FINISHED:
        return GLZW_OK;

    default:
        return GLZW_INTERNAL_ERROR;
    }
}

void glzwe_end(void *state)
{
    free(state);
}
