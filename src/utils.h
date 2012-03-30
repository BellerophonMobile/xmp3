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
 * @file utils.h
 * Miscellaneous utility functions.
 */

#pragma once

#include <stdbool.h>
#include <string.h>

/**
 * Allocate and copy a string.
 *
 * This makes sure the final string is null-terminated.
 *
 * @param[out] dst Pointer to store the string in.
 * @param[in] src Source string to copy.
 */
#define STRDUP_CHECK(dst, src) do { \
    (dst) = strdup(src); \
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
 * The size of the string returned by the make_uuid function.
 *
 * Includes the trailing null terminator.
 */
extern const size_t UUID_SIZE;

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

/** Decodes a base64 encoded string.
 *
 * Returned string is owned by the caller, must be freed.
 */
char* base64_decode(const char *input, int length);
