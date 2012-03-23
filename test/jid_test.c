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
 * @file jid_test.c
 * Unit tests for JID functions.
 */

#include <test-dept.h>
#include <stdlib.h>
#include <stdio.h>

#include "jid.h"

void setup(void) {
}

void teardown(void) {
    restore_function(&calloc);
}

static void* always_failing_calloc() {
    return NULL;
}

/** Tests return from jid_new when calloc fails. */
void test_new_bad_malloc(void) {
    replace_function(&calloc, &always_failing_calloc);
    struct jid* jid = jid_new();
    assert_pointer_equals(NULL, jid);
}

/** Tests a normal full JID. */
void test_from_str1(void) {
    struct jid* jid = jid_new_from_str("local@domain/resource");
    assert_string_equals("local", jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_string_equals("resource", jid_resource(jid));
    jid_del(jid);
}

/** Tests a normal JID without a resource. */
void test_from_str2(void) {
    struct jid* jid = jid_new_from_str("local@domain");
    assert_string_equals("local", jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests a normal JID without a local part or resource. */
void test_from_str3(void) {
    struct jid* jid = jid_new_from_str("domain");
    assert_pointer_equals(NULL, jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests an empty JID, should be an error. */
void test_from_str4(void) {
    /* An empty JID is an error, should return NULL. */
    struct jid* jid = jid_new_from_str("");
    assert_pointer_equals(NULL, jid);
}
