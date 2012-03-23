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
 * @file utils_test.c
 * Unit tests for utility functions.
 */

#include <stdlib.h>

#include <test-dept.h>

#include "utils.h"

void setup(void) {
}

void teardown(void) {
    restore_function(&malloc);
}

#if 0
static void* always_failing_malloc() {
    return NULL;
}
#endif

/** Tests generating a UUID.  Just makes sure it doesn't return NULL. */
void test_make_uuid(void) {
    char *uuid = make_uuid();
    assert_not_equals(NULL, uuid);
    free(uuid);
}

/** Tests generating a UUID when calloc fails. */
/* Causes the test to abort, with current use of check_mem. */
#if 0
void test_make_uuid_fail(void) {
    replace_function(&malloc, &always_failing_malloc);
    char *uuid = make_uuid();
    assert_not_equals(NULL, uuid);
    free(uuid);
}
#endif

/** Tests converting a string to an integer. */
void test_read_int1(void) {
    long int out;
    assert_true(read_int("10", &out));
    assert_equals(10, out);
}

/** Tests converting a string to a negative integer. */
void test_read_int2(void) {
    long int out;
    assert_true(read_int("-10", &out));
    assert_equals(-10, out);
}

/** Tests converting a string to a positive integer. */
void test_read_int3(void) {
    long int out;
    assert_true(read_int("+10", &out));
    assert_equals(10, out);
}

/** Tests converting an invalid string to an integer. */
void test_read_int4(void) {
    long int out;
    assert_false(read_int("10.5", &out));
}

/** Tests converting an invalid string to an integer. */
void test_read_int5(void) {
    long int out;
    assert_false(read_int("10a", &out));
}

/** Tests converting an invalid string to an integer. */
void test_read_int6(void) {
    long int out;
    assert_false(read_int("a10", &out));
}

/** Tests copying a string with no previous string set. */
void test_copy_string1(void) {
    static const char *src = "HELLO WORLD";
    char *dst = NULL;
    copy_string(&dst, src);
    assert_string_equals(src, dst);
    free(dst);
}

/** Tests copying a string with a previous string set. */
void test_copy_string2(void) {
    static const char *src = "HELLO WORLD";
    char *dst = strdup("FOOBAR");
    copy_string(&dst, src);
    assert_string_equals(src, dst);
    free(dst);
}

/** Tests clearing a string with no previous string set. */
void test_copy_string3(void) {
    char *dst = NULL;
    copy_string(&dst, NULL);
    assert_pointer_equals(NULL, dst);
}

/** Tests clearing a string with a previous string set. */
void test_copy_string4(void) {
    char *dst = strdup("FOOBAR");
    copy_string(&dst, NULL);
    assert_pointer_equals(NULL, dst);
}

/* Base64 tests from Section 10 of RFC4648. */

/** Tests decoding an empty base64 string. */
void test_base64_decode1(void) {
    static const char *input = "";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("", output);
    free(output);
}

/** Tests decoding a base64 string. */
void test_base64_decode2(void) {
    static const char *input = "Zg==";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("f", output);
    free(output);
}

/** Tests decoding a base64 string. */
void test_base64_decode3(void) {
    static const char *input = "Zm8=";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("fo", output);
    free(output);
}

/** Tests decoding a base64 string. */
void test_base64_decode4(void) {
    static const char *input = "Zm9v";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("foo", output);
    free(output);
}

/** Tests decoding a base64 string. */
void test_base64_decode5(void) {
    static const char *input = "Zm9vYg==";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("foob", output);
    free(output);
}

/** Tests decoding a base64 string. */
void test_base64_decode6(void) {
    static const char *input = "Zm9vYmE=";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("fooba", output);
    free(output);
}

/** Tests decoding a base64 string. */
void test_base64_decode7(void) {
    static const char *input = "Zm9vYmFy";
    char *output = base64_decode(input, strlen(input));
    assert_string_equals("foobar", output);
    free(output);
}
