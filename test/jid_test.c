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

/** Tests getting/setting localpart. */
void test_get_set_local(void) {
    struct jid* jid = jid_new();
    jid_set_local(jid, "local");
    assert_string_equals("local", jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests clearing localpart. */
void test_clear_local(void) {
    struct jid *jid = jid_new();
    jid_set_local(jid, "local");
    jid_set_local(jid, NULL);
    assert_pointer_equals(NULL, jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests resetting localpart. */
void test_reset_local(void) {
    struct jid *jid = jid_new();
    jid_set_local(jid, "local");
    jid_set_local(jid, "aaaaa");
    assert_string_equals("aaaaa", jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests getting/setting domainpart. */
void test_get_set_domain(void) {
    struct jid* jid = jid_new();
    jid_set_domain(jid, "domain");
    assert_pointer_equals(NULL, jid_local(jid));
    assert_string_equals("domain", jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests clearing domainpart. */
void test_clear_domain(void) {
    struct jid *jid = jid_new();
    jid_set_domain(jid, "domain");
    jid_set_domain(jid, NULL);
    assert_pointer_equals(NULL, jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests resetting domainpart. */
void test_reset_domain(void) {
    struct jid* jid = jid_new();
    jid_set_domain(jid, "domain");
    jid_set_domain(jid, "bbbbbb");
    assert_pointer_equals(NULL, jid_local(jid));
    assert_string_equals("bbbbbb", jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests getting/setting resourcepart. */
void test_get_set_resource(void) {
    struct jid* jid = jid_new();
    jid_set_resource(jid, "resource");
    assert_pointer_equals(NULL, jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_string_equals("resource", jid_resource(jid));
    jid_del(jid);
}

/** Tests clearing resourcepart. */
void test_clear_resource(void) {
    struct jid *jid = jid_new();
    jid_set_resource(jid, "resource");
    jid_set_resource(jid, NULL);
    assert_pointer_equals(NULL, jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_pointer_equals(NULL, jid_resource(jid));
    jid_del(jid);
}

/** Tests resetting resourcepart. */
void test_reset_resource(void) {
    struct jid* jid = jid_new();
    jid_set_resource(jid, "resource");
    jid_set_resource(jid, "cccccccc");
    assert_pointer_equals(NULL, jid_local(jid));
    assert_pointer_equals(NULL, jid_domain(jid));
    assert_string_equals("cccccccc", jid_resource(jid));
    jid_del(jid);
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
    struct jid* jid = jid_new_from_str("");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty localpart, should be an error. */
void test_from_str5(void) {
    struct jid* jid = jid_new_from_str("@domain");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty localpart, should be an error. */
void test_from_str6(void) {
    struct jid* jid = jid_new_from_str("@domain/resource");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty resourcepart, should be an error. */
void test_from_str7(void) {
    struct jid* jid = jid_new_from_str("domain/");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty resourcepart, should be an error. */
void test_from_str8(void) {
    struct jid* jid = jid_new_from_str("local@domain/");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty domain, should be an error. */
void test_from_str9(void) {
    struct jid* jid = jid_new_from_str("local@/resource");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty domain, should be an error. */
void test_from_str10(void) {
    struct jid* jid = jid_new_from_str("/resource");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty domain, should be an error. */
void test_from_str11(void) {
    struct jid* jid = jid_new_from_str("local@");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty everything, should be an error. */
void test_from_str12(void) {
    struct jid* jid = jid_new_from_str("@/");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty everything, should be an error. */
void test_from_str13(void) {
    struct jid* jid = jid_new_from_str("@");
    assert_pointer_equals(NULL, jid);
}

/** Tests an empty everything, should be an error. */
void test_from_str14(void) {
    struct jid* jid = jid_new_from_str("/");
    assert_pointer_equals(NULL, jid);
}

/** Tests items in the wrong order, should be an error. */
void test_from_str15(void) {
    struct jid* jid = jid_new_from_str("foo/bar@baz");
    assert_pointer_equals(NULL, jid);
}

/** Tests copying a full JID. */
void test_from_jid1(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_jid(a);
    assert_string_equals(jid_local(a), jid_local(b));
    assert_string_equals(jid_domain(a), jid_domain(b));
    assert_string_equals(jid_resource(a), jid_resource(b));
    jid_del(a);
    jid_del(b);
}

/** Tests copying a JID without a resource. */
void test_from_jid2(void) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_jid(a);
    assert_string_equals(jid_local(a), jid_local(b));
    assert_string_equals(jid_domain(a), jid_domain(b));
    assert_pointer_equals(jid_resource(a), jid_resource(b));
    jid_del(a);
    jid_del(b);
}

/** Tests copying a JID without a local part. */
void test_from_jid3(void) {
    struct jid *a = jid_new_from_str("domain/resource");
    struct jid *b = jid_new_from_jid(a);
    assert_pointer_equals(jid_local(a), jid_local(b));
    assert_string_equals(jid_domain(a), jid_domain(b));
    assert_string_equals(jid_resource(a), jid_resource(b));
    jid_del(a);
    jid_del(b);
}

/** Tests copying a JID without a local part or resource. */
void test_from_jid4(void) {
    struct jid *a = jid_new_from_str("domain");
    struct jid *b = jid_new_from_jid(a);
    assert_pointer_equals(jid_local(a), jid_local(b));
    assert_string_equals(jid_domain(a), jid_domain(b));
    assert_pointer_equals(jid_resource(a), jid_resource(b));
    jid_del(a);
    jid_del(b);
}

/** Tests converting a full JID to a bare JID. */
void test_from_jid_bare1(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_jid_bare(a);
    assert_string_equals(jid_local(a), jid_local(b));
    assert_string_equals(jid_domain(a), jid_domain(b));
    assert_pointer_equals(NULL, jid_resource(b));
    jid_del(a);
    jid_del(b);
}

/** Tests converting a bare JID to a bare JID. */
void test_from_jid_bare2(void) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_jid_bare(a);
    assert_string_equals(jid_local(a), jid_local(b));
    assert_string_equals(jid_domain(a), jid_domain(b));
    assert_pointer_equals(NULL, jid_resource(b));
    jid_del(a);
    jid_del(b);
}

/** Tests converting a JID back to a string. */
void test_to_str1(void) {
    static const char *JID = "local@domain/resource";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equals(JID, jid_str);
    free(jid_str);
    jid_del(a);
}

/** Tests converting a JID back to a string. */
void test_to_str2(void) {
    static const char *JID = "domain/resource";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equals(JID, jid_str);
    free(jid_str);
    jid_del(a);
}

/** Tests converting a JID back to a string. */
void test_to_str3(void) {
    static const char *JID = "local@domain";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equals(JID, jid_str);
    free(jid_str);
    jid_del(a);
}

/** Tests converting a JID back to a string. */
void test_to_str4(void) {
    static const char *JID = "domain";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equals(JID, jid_str);
    free(jid_str);
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len1(void) {
    static const char *JID = "local@domain/resource";
    struct jid *a = jid_new_from_str(JID);
    assert_equals(strlen(JID), jid_to_str_len(a));
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len2(void) {
    static const char *JID = "domain/resource";
    struct jid *a = jid_new_from_str(JID);
    assert_equals(strlen(JID), jid_to_str_len(a));
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len3(void) {
    static const char *JID = "local@domain";
    struct jid *a = jid_new_from_str(JID);
    assert_equals(strlen(JID), jid_to_str_len(a));
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len4(void) {
    static const char *JID = "domain";
    struct jid *a = jid_new_from_str(JID);
    assert_equals(strlen(JID), jid_to_str_len(a));
    jid_del(a);
}

/** Tests comparing equal JIDs. */
void test_cmp1(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs. */
void test_cmp2(void) {
    struct jid *a = jid_new_from_str("domain/resource");
    struct jid *b = jid_new_from_str("domain/resource");
    assert_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs. */
void test_cmp3(void) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_str("local@domain");
    assert_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs. */
void test_cmp4(void) {
    struct jid *a = jid_new_from_str("domain");
    struct jid *b = jid_new_from_str("domain");
    assert_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp5(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ddd");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp6(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd/ccc");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp7(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("ddd@bbb/ccc");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp8(void) {
    struct jid *a = jid_new_from_str("aaa@bbb");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp9(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp10(void) {
    struct jid *a = jid_new_from_str("bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp11(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("bbb/ccc");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp12(void) {
    struct jid *a = jid_new_from_str("bbb");
    struct jid *b = jid_new_from_str("ccc");
    assert_not_equals(0, jid_cmp(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards1(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards2(void) {
    struct jid *a = jid_new_from_str("domain/resource");
    struct jid *b = jid_new_from_str("domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards3(void) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_str("local@domain");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards4(void) {
    struct jid *a = jid_new_from_str("domain");
    struct jid *b = jid_new_from_str("domain");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards5(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/*");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards6(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@*/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards7(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards8(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@*/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards9(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@domain/*");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards10(void) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@*/*");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards11(void) {
    struct jid *a = jid_new_from_str("local@domain/*");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards12(void) {
    struct jid *a = jid_new_from_str("local@*/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards13(void) {
    struct jid *a = jid_new_from_str("*@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards14(void) {
    struct jid *a = jid_new_from_str("*@*/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards15(void) {
    struct jid *a = jid_new_from_str("*@domain/*");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards16(void) {
    struct jid *a = jid_new_from_str("*@*/*");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards17(void) {
    struct jid *a = jid_new_from_str("*@*/*");
    struct jid *b = jid_new_from_str("*@*/*");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards18(void) {
    struct jid *a = jid_new_from_str("local@*/*");
    struct jid *b = jid_new_from_str("local@*/resource");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing a bare JID with a full JID, matches with wildcards. */
void test_cmp_wildcards19(void) {
    struct jid *a = jid_new_from_str("aaa@bbb");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing a bare JID with a full JID, matches with wildcards. */
void test_cmp_wildcards20(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb");
    assert_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards21(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ddd");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards22(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd/ccc");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards23(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("ddd@bbb/ccc");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards24(void) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards25(void) {
    struct jid *a = jid_new_from_str("bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards26(void) {
    struct jid *a = jid_new_from_str("*@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd/ccc");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards27(void) {
    struct jid *a = jid_new_from_str("ddd@*/ccc");
    struct jid *b = jid_new_from_str("aaa@*/ccc");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards28(void) {
    struct jid *a = jid_new_from_str("*@*/ddd");
    struct jid *b = jid_new_from_str("*@*/ccc");
    assert_not_equals(0, jid_cmp_wildcards(a, b));
    jid_del(a);
    jid_del(b);
}
