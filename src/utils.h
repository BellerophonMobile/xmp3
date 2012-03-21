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

/** Decodes a base64 encoded string.
 *
 * Returned string is owned by the caller, must be freed.
 */
char* base64_decode(const char *input, int length);
