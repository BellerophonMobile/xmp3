/*
 * Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file utils.c
 * Miscellaneous utility functions
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "log.h"
#include "utils.h"

enum base64_decodestep {
    base64_step_a,
    base64_step_b,
    base64_step_c,
    base64_step_d,
};

struct base64_decodestate {
    enum base64_decodestep step;
    char plainchar;
};

/* Forward declarations */
static void base64_init_decodestate(struct base64_decodestate *state_in);
static int base64_decode_value(char value_in);
static int base64_decode_block(const char *code_in, const int length_in,
                               char *plaintext_out,
                               struct base64_decodestate *state_in);

char* make_uuid(void) {
    uuid_t uuid;
    uuid_generate(uuid);
    char *rv = malloc(37 * sizeof(char));
    uuid_unparse(uuid, rv);
    return rv;
}

bool read_int(const char *arg, long int *output) {
    // Clear errno so we can check if strtol fails.
    errno = 0;

    char *endptr;
    *output = strtol(arg, &endptr, 10);

    // We want to make sure we read the whole string.
    return (errno != ERANGE) && (*endptr == '\0');
}

void copy_string(char **dest, const char *src) {
    if (src == NULL) {
        if (*dest != NULL) {
            free(*dest);
            *dest = NULL;
        }
        return;
    }

    if (*dest != NULL) {
        int size = strlen(src) + 1;
        char *new_dest = realloc(*dest, size);
        check_mem(new_dest);
        *dest = new_dest;
        memcpy(*dest, src, size);
    } else {
        STRDUP_CHECK(*dest, src);
    }
}

char* base64_decode(const char *input, int length) {
    int plain_len = length * 0.75 + 1;
    char *plaintext = malloc(plain_len * sizeof(char));
    check_mem(plaintext);

    struct base64_decodestate state;
    base64_init_decodestate(&state);
    base64_decode_block(input, length, plaintext, &state);

    plaintext[plain_len - 1] = '\0';

    return plaintext;
}

/*
 * These functions are from libb64, found at: http://libb64.sourceforge.net/
 *
 * cdecoder.c - c source to a base64 decoding algorithm implementation
 *
 * This is part of the libb64 project, and has been placed in the public
 * domain. For details, see http://sourceforge.net/projects/libb64
 */
static void base64_init_decodestate(struct base64_decodestate *state_in) {
    state_in->step = base64_step_a;
    state_in->plainchar = 0;
}

static int base64_decode_value(char value_in) {
    static const char decoding[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1,
        -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
    static const char decoding_size = sizeof(decoding);
    value_in -= 43;
    if (value_in < 0 || value_in > decoding_size) {
        return -1;
    }
    return decoding[(int)value_in];
}

static int base64_decode_block(const char *code_in, const int length_in,
                               char *plaintext_out,
                               struct base64_decodestate *state_in) {
    const char *codechar = code_in;
    char *plainchar = plaintext_out;
    char fragment;

    *plainchar = state_in->plainchar;

    switch (state_in->step) {
        while (1) {
            case base64_step_a:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_a;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar = (fragment & 0x03f) << 2;
            case base64_step_b:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_b;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x030) >> 4;
                *plainchar = (fragment & 0x00f) << 4;
            case base64_step_c:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_c;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x03c) >> 2;
                *plainchar = (fragment & 0x003) << 6;
            case base64_step_d:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_d;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x03f);
        }
    }
    /* control should not reach here */
    return plainchar - plaintext_out;
}
