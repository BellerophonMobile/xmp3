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

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmockery.h>

#include "jid.c"

/** Tests getting/setting localpart. */
void test_get_set_local(void **state) {
    struct jid* jid = jid_new();
    jid_set_local(jid, "local");
    assert_string_equal(jid_local(jid), "local");
    assert_true(jid_domain(jid) == NULL);
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests clearing localpart. */
void test_clear_local(void **state) {
    struct jid *jid = jid_new();
    jid_set_local(jid, "local");
    jid_set_local(jid, NULL);
    assert_true(jid_local(jid) == NULL);
    assert_true(jid_domain(jid) == NULL);
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests resetting localpart. */
void test_reset_local(void **state) {
    struct jid *jid = jid_new();
    jid_set_local(jid, "local");
    jid_set_local(jid, "aaaaa");
    assert_string_equal(jid_local(jid), "aaaaa");
    assert_true(jid_domain(jid) == NULL);
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests getting/setting domainpart. */
void test_get_set_domain(void **state) {
    struct jid* jid = jid_new();
    jid_set_domain(jid, "domain");
    assert_true(jid_local(jid) == NULL);
    assert_string_equal(jid_domain(jid), "domain");
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests clearing domainpart. */
void test_clear_domain(void **state) {
    struct jid *jid = jid_new();
    jid_set_domain(jid, "domain");
    jid_set_domain(jid, NULL);
    assert_true(jid_local(jid) == NULL);
    assert_true(jid_domain(jid) == NULL);
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests resetting domainpart. */
void test_reset_domain(void **state) {
    struct jid* jid = jid_new();
    jid_set_domain(jid, "domain");
    jid_set_domain(jid, "bbbbbb");
    assert_true(jid_local(jid) == NULL);
    assert_string_equal(jid_domain(jid), "bbbbbb");
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests getting/setting resourcepart. */
void test_get_set_resource(void **state) {
    struct jid* jid = jid_new();
    jid_set_resource(jid, "resource");
    assert_true(jid_local(jid) == NULL);
    assert_true(jid_domain(jid) == NULL);
    assert_string_equal(jid_resource(jid), "resource");
    jid_del(jid);
}

/** Tests clearing resourcepart. */
void test_clear_resource(void **state) {
    struct jid *jid = jid_new();
    jid_set_resource(jid, "resource");
    jid_set_resource(jid, NULL);
    assert_true(jid_local(jid) == NULL);
    assert_true(jid_domain(jid) == NULL);
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests resetting resourcepart. */
void test_reset_resource(void **state) {
    struct jid* jid = jid_new();
    jid_set_resource(jid, "resource");
    jid_set_resource(jid, "cccccccc");
    assert_true(jid_local(jid) == NULL);
    assert_true(jid_domain(jid) == NULL);
    assert_string_equal(jid_resource(jid), "cccccccc");
    jid_del(jid);
}

/** Tests a normal full JID. */
void test_from_str1(void **state) {
    struct jid* jid = jid_new_from_str("local@domain/resource");
    assert_string_equal(jid_local(jid), "local");
    assert_string_equal(jid_domain(jid), "domain");
    assert_string_equal(jid_resource(jid), "resource");
    jid_del(jid);
}

/** Tests a normal JID without a resource. */
void test_from_str2(void **state) {
    struct jid* jid = jid_new_from_str("local@domain");
    assert_string_equal(jid_local(jid), "local");
    assert_string_equal(jid_domain(jid), "domain");
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests a normal JID without a local part or resource. */
void test_from_str3(void **state) {
    struct jid* jid = jid_new_from_str("domain");
    assert_true(jid_local(jid) == NULL);
    assert_string_equal(jid_domain(jid), "domain");
    assert_true(jid_resource(jid) == NULL);
    jid_del(jid);
}

/** Tests an empty JID, should be an error. */
void test_from_str4(void **state) {
    struct jid* jid = jid_new_from_str("");
    assert_true(jid == NULL);
}

/** Tests an empty localpart, should be an error. */
void test_from_str5(void **state) {
    struct jid* jid = jid_new_from_str("@domain");
    assert_true(jid == NULL);
}

/** Tests an empty localpart, should be an error. */
void test_from_str6(void **state) {
    struct jid* jid = jid_new_from_str("@domain/resource");
    assert_true(jid == NULL);
}

/** Tests an empty resourcepart, should be an error. */
void test_from_str7(void **state) {
    struct jid* jid = jid_new_from_str("domain/");
    assert_true(jid == NULL);
}

/** Tests an empty resourcepart, should be an error. */
void test_from_str8(void **state) {
    struct jid* jid = jid_new_from_str("local@domain/");
    assert_true(jid == NULL);
}

/** Tests an empty domain, should be an error. */
void test_from_str9(void **state) {
    struct jid* jid = jid_new_from_str("local@/resource");
    assert_true(jid == NULL);
}

/** Tests an empty domain, should be an error. */
void test_from_str10(void **state) {
    struct jid* jid = jid_new_from_str("/resource");
    assert_true(jid == NULL);
}

/** Tests an empty domain, should be an error. */
void test_from_str11(void **state) {
    struct jid* jid = jid_new_from_str("local@");
    assert_true(jid == NULL);
}

/** Tests an empty everything, should be an error. */
void test_from_str12(void **state) {
    struct jid* jid = jid_new_from_str("@/");
    assert_true(jid == NULL);
}

/** Tests an empty everything, should be an error. */
void test_from_str13(void **state) {
    struct jid* jid = jid_new_from_str("@");
    assert_true(jid == NULL);
}

/** Tests an empty everything, should be an error. */
void test_from_str14(void **state) {
    struct jid* jid = jid_new_from_str("/");
    assert_true(jid == NULL);
}

/** Tests items in the wrong order, should be an error. */
void test_from_str15(void **state) {
    struct jid* jid = jid_new_from_str("foo/bar@baz");
    assert_true(jid == NULL);
}

/** Tests copying a full JID. */
void test_from_jid1(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_jid(a);
    assert_string_equal(jid_local(b), jid_local(a));
    assert_string_equal(jid_domain(b), jid_domain(a));
    assert_string_equal(jid_resource(b), jid_resource(a));
    jid_del(a);
    jid_del(b);
}

/** Tests copying a JID without a resource. */
void test_from_jid2(void **state) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_jid(a);
    assert_string_equal(jid_local(b), jid_local(a));
    assert_string_equal(jid_domain(b), jid_domain(a));
    assert_true(jid_resource(b) == jid_resource(a));
    jid_del(a);
    jid_del(b);
}

/** Tests copying a JID without a local part. */
void test_from_jid3(void **state) {
    struct jid *a = jid_new_from_str("domain/resource");
    struct jid *b = jid_new_from_jid(a);
    assert_true(jid_local(b) == jid_local(a));
    assert_string_equal(jid_domain(b), jid_domain(a));
    assert_string_equal(jid_resource(b), jid_resource(a));
    jid_del(a);
    jid_del(b);
}

/** Tests copying a JID without a local part or resource. */
void test_from_jid4(void **state) {
    struct jid *a = jid_new_from_str("domain");
    struct jid *b = jid_new_from_jid(a);
    assert_true(jid_local(b) == jid_local(a));
    assert_string_equal(jid_domain(b), jid_domain(a));
    assert_true(jid_resource(b) == jid_resource(a));
    jid_del(a);
    jid_del(b);
}

/** Tests converting a full JID to a bare JID. */
void test_from_jid_bare1(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_jid_bare(a);
    assert_string_equal(jid_local(b), jid_local(a));
    assert_string_equal(jid_domain(b), jid_domain(a));
    assert_true(jid_resource(b) == NULL);
    jid_del(a);
    jid_del(b);
}

/** Tests converting a bare JID to a bare JID. */
void test_from_jid_bare2(void **state) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_jid_bare(a);
    assert_string_equal(jid_local(b), jid_local(a));
    assert_string_equal(jid_domain(b), jid_domain(a));
    assert_true(jid_resource(b) == NULL);
    jid_del(a);
    jid_del(b);
}

/** Tests converting a JID back to a string. */
void test_to_str1(void **state) {
    static const char *JID = "local@domain/resource";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equal(jid_str, JID);
    free(jid_str);
    jid_del(a);
}

/** Tests converting a JID back to a string. */
void test_to_str2(void **state) {
    static const char *JID = "domain/resource";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equal(jid_str, JID);
    free(jid_str);
    jid_del(a);
}

/** Tests converting a JID back to a string. */
void test_to_str3(void **state) {
    static const char *JID = "local@domain";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equal(jid_str, JID);
    free(jid_str);
    jid_del(a);
}

/** Tests converting a JID back to a string. */
void test_to_str4(void **state) {
    static const char *JID = "domain";
    struct jid *a = jid_new_from_str(JID);
    char *jid_str = jid_to_str(a);
    assert_string_equal(jid_str, JID);
    free(jid_str);
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len1(void **state) {
    static const char *JID = "local@domain/resource";
    struct jid *a = jid_new_from_str(JID);
    assert_int_equal(jid_to_str_len(a), strlen(JID));
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len2(void **state) {
    static const char *JID = "domain/resource";
    struct jid *a = jid_new_from_str(JID);
    assert_int_equal(jid_to_str_len(a), strlen(JID));
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len3(void **state) {
    static const char *JID = "local@domain";
    struct jid *a = jid_new_from_str(JID);
    assert_int_equal(jid_to_str_len(a), strlen(JID));
    jid_del(a);
}

/** Tests checking the length of a JID as a string. */
void test_to_str_len4(void **state) {
    static const char *JID = "domain";
    struct jid *a = jid_new_from_str(JID);
    assert_int_equal(jid_to_str_len(a), strlen(JID));
    jid_del(a);
}

/** Tests comparing equal JIDs. */
void test_cmp1(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs. */
void test_cmp2(void **state) {
    struct jid *a = jid_new_from_str("domain/resource");
    struct jid *b = jid_new_from_str("domain/resource");
    assert_int_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs. */
void test_cmp3(void **state) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_str("local@domain");
    assert_int_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs. */
void test_cmp4(void **state) {
    struct jid *a = jid_new_from_str("domain");
    struct jid *b = jid_new_from_str("domain");
    assert_int_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp5(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ddd");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp6(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd/ccc");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp7(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("ddd@bbb/ccc");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp8(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp9(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp10(void **state) {
    struct jid *a = jid_new_from_str("bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp11(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("bbb/ccc");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs. */
void test_cmp12(void **state) {
    struct jid *a = jid_new_from_str("bbb");
    struct jid *b = jid_new_from_str("ccc");
    assert_int_not_equal(jid_cmp(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards1(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards2(void **state) {
    struct jid *a = jid_new_from_str("domain/resource");
    struct jid *b = jid_new_from_str("domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards3(void **state) {
    struct jid *a = jid_new_from_str("local@domain");
    struct jid *b = jid_new_from_str("local@domain");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards4(void **state) {
    struct jid *a = jid_new_from_str("domain");
    struct jid *b = jid_new_from_str("domain");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards5(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/*");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards6(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("local@*/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards7(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards8(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@*/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards9(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@domain/*");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards10(void **state) {
    struct jid *a = jid_new_from_str("local@domain/resource");
    struct jid *b = jid_new_from_str("*@*/*");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards11(void **state) {
    struct jid *a = jid_new_from_str("local@domain/*");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards12(void **state) {
    struct jid *a = jid_new_from_str("local@*/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards13(void **state) {
    struct jid *a = jid_new_from_str("*@domain/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards14(void **state) {
    struct jid *a = jid_new_from_str("*@*/resource");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards15(void **state) {
    struct jid *a = jid_new_from_str("*@domain/*");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards16(void **state) {
    struct jid *a = jid_new_from_str("*@*/*");
    struct jid *b = jid_new_from_str("local@domain/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards17(void **state) {
    struct jid *a = jid_new_from_str("*@*/*");
    struct jid *b = jid_new_from_str("*@*/*");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing equal JIDs with wildcards. */
void test_cmp_wildcards18(void **state) {
    struct jid *a = jid_new_from_str("local@*/*");
    struct jid *b = jid_new_from_str("local@*/resource");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing a bare JID with a full JID, matches with wildcards. */
void test_cmp_wildcards19(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing a bare JID with a full JID, matches with wildcards. */
void test_cmp_wildcards20(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb");
    assert_int_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards21(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ddd");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards22(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd/ccc");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards23(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("ddd@bbb/ccc");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards24(void **state) {
    struct jid *a = jid_new_from_str("aaa@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards25(void **state) {
    struct jid *a = jid_new_from_str("bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@bbb/ccc");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards26(void **state) {
    struct jid *a = jid_new_from_str("*@bbb/ccc");
    struct jid *b = jid_new_from_str("aaa@ddd/ccc");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards27(void **state) {
    struct jid *a = jid_new_from_str("ddd@*/ccc");
    struct jid *b = jid_new_from_str("aaa@*/ccc");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

/** Tests comparing non-equal JIDs with wildcards. */
void test_cmp_wildcards28(void **state) {
    struct jid *a = jid_new_from_str("*@*/ddd");
    struct jid *b = jid_new_from_str("*@*/ccc");
    assert_int_not_equal(jid_cmp_wildcards(a, b), 0);
    jid_del(a);
    jid_del(b);
}

int main(int argc, char *argv[]) {
    const UnitTest tests[] = {
        unit_test(test_get_set_local),
        unit_test(test_clear_local),
        unit_test(test_reset_local),
        unit_test(test_get_set_domain),
        unit_test(test_clear_domain),
        unit_test(test_reset_domain),
        unit_test(test_get_set_resource),
        unit_test(test_clear_resource),
        unit_test(test_reset_resource),
        unit_test(test_from_str1),
        unit_test(test_from_str2),
        unit_test(test_from_str3),
        unit_test(test_from_str4),
        unit_test(test_from_str5),
        unit_test(test_from_str6),
        unit_test(test_from_str7),
        unit_test(test_from_str8),
        unit_test(test_from_str9),
        unit_test(test_from_str10),
        unit_test(test_from_str11),
        unit_test(test_from_str12),
        unit_test(test_from_str13),
        unit_test(test_from_str14),
        unit_test(test_from_str15),
        unit_test(test_from_jid1),
        unit_test(test_from_jid2),
        unit_test(test_from_jid3),
        unit_test(test_from_jid4),
        unit_test(test_from_jid_bare1),
        unit_test(test_from_jid_bare2),
        unit_test(test_to_str1),
        unit_test(test_to_str2),
        unit_test(test_to_str3),
        unit_test(test_to_str4),
        unit_test(test_to_str_len1),
        unit_test(test_to_str_len2),
        unit_test(test_to_str_len3),
        unit_test(test_to_str_len4),
        unit_test(test_cmp1),
        unit_test(test_cmp2),
        unit_test(test_cmp3),
        unit_test(test_cmp4),
        unit_test(test_cmp5),
        unit_test(test_cmp6),
        unit_test(test_cmp7),
        unit_test(test_cmp8),
        unit_test(test_cmp9),
        unit_test(test_cmp10),
        unit_test(test_cmp11),
        unit_test(test_cmp12),
        unit_test(test_cmp_wildcards1),
        unit_test(test_cmp_wildcards2),
        unit_test(test_cmp_wildcards3),
        unit_test(test_cmp_wildcards4),
        unit_test(test_cmp_wildcards5),
        unit_test(test_cmp_wildcards6),
        unit_test(test_cmp_wildcards7),
        unit_test(test_cmp_wildcards8),
        unit_test(test_cmp_wildcards9),
        unit_test(test_cmp_wildcards10),
        unit_test(test_cmp_wildcards11),
        unit_test(test_cmp_wildcards12),
        unit_test(test_cmp_wildcards13),
        unit_test(test_cmp_wildcards14),
        unit_test(test_cmp_wildcards15),
        unit_test(test_cmp_wildcards16),
        unit_test(test_cmp_wildcards17),
        unit_test(test_cmp_wildcards18),
        unit_test(test_cmp_wildcards19),
        unit_test(test_cmp_wildcards20),
        unit_test(test_cmp_wildcards21),
        unit_test(test_cmp_wildcards22),
        unit_test(test_cmp_wildcards23),
        unit_test(test_cmp_wildcards24),
        unit_test(test_cmp_wildcards25),
        unit_test(test_cmp_wildcards26),
        unit_test(test_cmp_wildcards27),
        unit_test(test_cmp_wildcards28),
    };
    return run_tests(tests);
}
