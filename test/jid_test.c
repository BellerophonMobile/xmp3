/**
 * xmp3 - XMPP Proxy
 * Copyright (c) 2012 Drexel University
 *
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

void test_new_bad_malloc(void) {
    replace_function(&calloc, &always_failing_calloc);
    struct jid* jid = jid_new();
    assert_pointer_equals(NULL, jid);
}

void test_from_str1(void) {
    struct jid* jid = jid_new_from_str("local@domain/resource");
    assert_string_equals("local", jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_string_equals("resource", jid_resource(jid));
    jid_del(jid);
}

void test_from_str2(void) {
    struct jid* jid = jid_new_from_str("local@domain");
    assert_string_equals("local", jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

void test_from_str3(void) {
    struct jid* jid = jid_new_from_str("domain");
    assert_pointer_equals(NULL, jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}
