/**
 * xmp3 - XMPP Proxy
 * utils.{c,h} - Miscellaneous utility functions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

/**
 * Allocate and copy a string.
 *
 * This makes sure the final string is null-terminated.
 *
 * @param[out] dst Pointer to store the string in.
 * @param[in] src Source string to copy.
 */
#define STRDUP_CHECK(dst, src) do { \
    dst = strdup(src); \
    check_mem(dst); \
} while (0)

/**
 * Allocate and copy a string, up to size n.
 *
 * This makes sure the final string is null-terminated.
 *
 * @param[out] dst Pointer to store the string in.
 * @param[in] src  Source string to copy.
 * @param[in] n    Number of characters to copy.
 */
#define STRNDUP_CHECK(dst, src, n) do { \
    dst = malloc((n + 1) * sizeof(char)); \
    check_mem(dst); \
    dst = strncpy(dst, src, n); \
    (dst)[n] = '\0'; \
} while(0)

/**
 * Allocate and return a null-terminated string with a UUID.
 */
char* make_uuid(void);

/**
 * Converts a string to an integer with error checking.
 *
 * @param[in]  arg    String value to convert.
 * @param[out] output Where to write the converted integer.
 * @return true if string is valid and conversion is successful, false
 *              otherwise.
 */
bool read_int(const char *arg, long int *output);

/** Simple string copy, freeing original if neccessary. */
void copy_string(char **dest, const char *src);

/*
 * These functions are from libb64, found at: http://libb64.sourceforge.net/
 *
 * cdecode.h - c header for a base64 decoding algorithm
 * This is part of the libb64 project, and has been placed in the public
 * domain.  For details, see http://sourceforge.net/projects/libb64
 */

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

void base64_init_decodestate(struct base64_decodestate *state_in);

int base64_decode_value(char value_in);

int base64_decode_block(const char *code_in, const int length_in,
                        char *plaintext_out,
                        struct base64_decodestate *state_in);
