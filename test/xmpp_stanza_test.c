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
 * @file xmpp_stanza_test.c
 * Unit tests for XMPP stanza functions.
 */

#include <stdlib.h>

#include <test-dept.h>

#include "xmpp_stanza.h"

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

/** Tests getting the tag name of a stanza. */
void test_name1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    assert_string_equals("message", xmpp_stanza_name(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the tag name of a stanza in a namespace. */
void test_name2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri message", NULL);
    assert_string_equals("message", xmpp_stanza_name(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the tag name of a stanza in a namespace and prefix. */
void test_name3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri message prefix", NULL);
    assert_string_equals("message", xmpp_stanza_name(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the namespace uri of a stanza. */
void test_uri1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_uri(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the namespace uri of a stanza in a namespace. */
void test_uri2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri message", NULL);
    assert_string_equals("uri", xmpp_stanza_uri(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the namespace uri of a stanza in a namespace and prefix. */
void test_uri3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri message prefix", NULL);
    assert_string_equals("uri", xmpp_stanza_uri(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the namespace prefix of a stanza. */
void test_prefix1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_prefix(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the namespace prefix of a stanza in a namespace. */
void test_prefix2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri message", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_prefix(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting the namespace prefix of a stanza in a namespace and prefix.*/
void test_prefix3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri message prefix", NULL);
    assert_string_equals("prefix", xmpp_stanza_prefix(a));
    xmpp_stanza_del(a, true);
}

/** Tests getting attributes from a stanza. */
void test_attr1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "foo", "bar",
        "bin", "baz",
        NULL,
    });
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests getting attributes from a stanza. */
void test_attr2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "foo", "bar",
        "bin", "baz",
        NULL,
    });
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    assert_string_equals("baz", xmpp_stanza_attr(a, "bin"));
    assert_pointer_equals(NULL, xmpp_stanza_attr(a, "asdfasdf"));
    xmpp_stanza_del(a, true);
}

/** Tests getting non-existent attributes from a stanza. */
void test_attr3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "foo", "bar",
        "bin", "baz",
        NULL,
    });
    assert_pointer_equals(NULL, xmpp_stanza_attr(a, "asdfasdf"));
    xmpp_stanza_del(a, true);
}

/** Tests setting an attribute on a stanza. */
void test_set_attr1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_set_attr(a, "foo", strdup("bar"));
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests setting an attribute on a stanza that already has that attribute. */
void test_set_attr2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "foo", "baz",
        NULL,
    });
    assert_string_equals("baz", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_set_attr(a, "foo", strdup("bar"));
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests clearing an attribute on a stanza. */
void test_set_attr3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_set_attr(a, "foo", strdup("bar"));
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_set_attr(a, "foo", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests clearing an attribute on a stanza. */
void test_set_attr4(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "foo", "bar",
        NULL,
    });
    xmpp_stanza_set_attr(a, "foo", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests clearing an attribute on a stanza. */
void test_set_attr5(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_set_attr(a, "foo", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests copying an attribute on a stanza. */
void test_copy_attr1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_copy_attr(a, "foo", "bar");
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests copying an attribute on a stanza that already has that attribute. */
void test_copy_attr2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "foo", "baz",
        NULL,
    });
    assert_string_equals("baz", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_copy_attr(a, "foo", "bar");
    assert_string_equals("bar", xmpp_stanza_attr(a, "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests getting attributes with namespaces from a stanza. */
void test_ns_attr1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "ns1 foo", "bar",
        "ns2 bin", "baz",
        NULL,
    });
    assert_string_equals("bar", xmpp_stanza_ns_attr(a, "foo", "ns1"));
    xmpp_stanza_del(a, true);
}

/** Tests getting attributes with namespaces from a stanza. */
void test_ns_attr2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "ns1 foo", "bar",
        "ns2 bin", "baz",
        NULL,
    });
    assert_string_equals("baz", xmpp_stanza_ns_attr(a, "bin", "ns2"));
    xmpp_stanza_del(a, true);
}

/** Tests getting non-existent attributes with namespaces from a stanza. */
void test_ns_attr3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "ns1 foo", "bar",
        "ns2 bin", "baz",
        NULL,
    });
    assert_pointer_equals(NULL, xmpp_stanza_ns_attr(a, "asdf", "asdfasdf"));
    xmpp_stanza_del(a, true);
}

/** Tests getting non-existent attributes with namespaces from a stanza. */
void test_ns_attr4(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "ns1 foo", "bar",
        "ns2 bin", "baz",
        NULL,
    });
    assert_pointer_equals(NULL, xmpp_stanza_ns_attr(a, "ns1", "bin"));
    xmpp_stanza_del(a, true);
}

/** Tests getting non-existent attributes with namespaces from a stanza. */
void test_ns_attr5(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "ns1 foo", "bar",
        "ns2 bin", "baz",
        NULL,
    });
    assert_pointer_equals(NULL, xmpp_stanza_ns_attr(a, "ns2", "foo"));
    xmpp_stanza_del(a, true);
}

/** Tests setting an attribute with a namespace on a stanza. */
void test_set_ns_attr1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_set_ns_attr(a, "foo", "uri", "prefix", strdup("bar"));
    assert_string_equals("bar", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests setting an attribute with a namespace on a stanza that already has
 *  that attribute. */
void test_set_ns_attr2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "uri foo", "baz",
        NULL,
    });
    assert_string_equals("baz", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_set_ns_attr(a, "foo", "uri", "prefix", strdup("bar"));
    assert_string_equals("bar", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests clearing an attribute with a namespace on a stanza. */
void test_set_ns_attr3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_set_ns_attr(a, "foo", "uri", "prefix", strdup("bar"));
    assert_string_equals("bar", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_set_ns_attr(a, "foo", "uri", "prefix", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests clearing an attribute with a namespace on a stanza. */
void test_set_ns_attr4(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "uri foo prefix", "bar",
        NULL,
    });
    xmpp_stanza_set_ns_attr(a, "foo", "uri", "prefix", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests clearing an attribute with a namespace on a stanza. */
void test_set_ns_attr5(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_set_ns_attr(a, "foo", "uri", "prefix", NULL);
    assert_pointer_equals(NULL, xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests copying an attribute with a namespace on a stanza. */
void test_copy_ns_attr1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    xmpp_stanza_copy_ns_attr(a, "foo", "uri", "prefix", "bar");
    assert_string_equals("bar", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests copying an attribute with a namespace on a stanza that already has
 *  that attribute. */
void test_copy_ns_attr2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", (const char*[]){
        "uri foo", "baz",
        NULL,
    });
    assert_string_equals("baz", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_copy_ns_attr(a, "foo", "uri", "prefix", "bar");
    assert_string_equals("bar", xmpp_stanza_ns_attr(a, "foo", "uri"));
    xmpp_stanza_del(a, true);
}

/** Tests appending character data to an XMPP stanza. */
void test_append_data1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    static const char *DATA = "HELLO WORLD";
    xmpp_stanza_append_data(a, DATA, strlen(DATA));
    assert_string_equals(DATA, xmpp_stanza_data(a));
    xmpp_stanza_del(a, true);
}

/** Tests appending multiple datas to an XMPP stanza. */
void test_append_data2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    static const char *DATA1 = "HELLO";
    static const char *DATA2 = " WORLD";
    static const char *DATA = "HELLO WORLD";
    xmpp_stanza_append_data(a, DATA1, strlen(DATA1));
    xmpp_stanza_append_data(a, DATA2, strlen(DATA2));
    assert_string_equals(DATA, xmpp_stanza_data(a));
    xmpp_stanza_del(a, true);
}

/** Tests appending data to an XMPP stanza and checking the length. */
void test_data_length1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    static const char *DATA = "HELLO WORLD";
    xmpp_stanza_append_data(a, DATA, strlen(DATA));
    assert_equals(strlen(DATA), xmpp_stanza_data_length(a));
    xmpp_stanza_del(a, true);
}

/** Tests appending multiple datas to an XMPP stanza and checking the length.*/
void test_data_length(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("message", NULL);
    static const char *DATA1 = "HELLO";
    static const char *DATA2 = " WORLD";
    static const char *DATA = "HELLO WORLD";
    xmpp_stanza_append_data(a, DATA1, strlen(DATA1));
    xmpp_stanza_append_data(a, DATA2, strlen(DATA2));
    assert_equals(strlen(DATA), xmpp_stanza_data_length(a));
    xmpp_stanza_del(a, true);
}

/** Tests appending children to XMPP stanzas. */
void test_append_child1(void) {
    struct xmpp_stanza *parent = xmpp_stanza_new("parent", NULL);
    struct xmpp_stanza *child = xmpp_stanza_new("child", NULL);

    xmpp_stanza_append_child(parent, child);

    assert_equals(1, xmpp_stanza_children_length(parent));
    assert_pointer_equals(child, xmpp_stanza_children(parent));
    assert_pointer_equals(parent, xmpp_stanza_parent(child));

    xmpp_stanza_del(parent, true);
}

/** Tests appending multiple children to XMPP stanzas. */
void test_append_child2(void) {
    struct xmpp_stanza *parent = xmpp_stanza_new("parent", NULL);
    struct xmpp_stanza *child1 = xmpp_stanza_new("child1", NULL);
    struct xmpp_stanza *child2 = xmpp_stanza_new("child2", NULL);
    struct xmpp_stanza *child3 = xmpp_stanza_new("child3", NULL);

    xmpp_stanza_append_child(parent, child1);
    xmpp_stanza_append_child(parent, child2);
    xmpp_stanza_append_child(parent, child3);

    assert_equals(3, xmpp_stanza_children_length(parent));
    assert_pointer_equals(child1, xmpp_stanza_children(parent));
    assert_pointer_equals(child2, xmpp_stanza_next(child1));
    assert_pointer_equals(child3, xmpp_stanza_next(child2));

    assert_pointer_equals(parent, xmpp_stanza_parent(child1));
    assert_pointer_equals(parent, xmpp_stanza_parent(child2));
    assert_pointer_equals(parent, xmpp_stanza_parent(child3));

    xmpp_stanza_del(parent, true);
}

/** Tests appending children to different XMPP stanzas. */
void test_append_child3(void) {
    struct xmpp_stanza *parent1 = xmpp_stanza_new("parent1", NULL);
    struct xmpp_stanza *parent2 = xmpp_stanza_new("parent2", NULL);
    struct xmpp_stanza *child1 = xmpp_stanza_new("child1", NULL);

    xmpp_stanza_append_child(parent1, child1);
    xmpp_stanza_append_child(parent2, child1);

    assert_equals(0, xmpp_stanza_children_length(parent1));
    assert_pointer_equals(NULL, xmpp_stanza_children(parent1));
    assert_equals(1, xmpp_stanza_children_length(parent2));
    assert_pointer_equals(child1, xmpp_stanza_children(parent2));

    assert_pointer_equals(parent2, xmpp_stanza_parent(child1));

    xmpp_stanza_del(parent1, true);
    xmpp_stanza_del(parent2, true);
}

/** Tests removing children from XMPP stanzas. */
void test_remove_child1(void) {
    struct xmpp_stanza *parent = xmpp_stanza_new("parent", NULL);
    struct xmpp_stanza *child1 = xmpp_stanza_new("child1", NULL);
    struct xmpp_stanza *child2 = xmpp_stanza_new("child2", NULL);
    struct xmpp_stanza *child3 = xmpp_stanza_new("child3", NULL);

    xmpp_stanza_append_child(parent, child1);
    xmpp_stanza_append_child(parent, child2);
    xmpp_stanza_append_child(parent, child3);

    xmpp_stanza_remove_child(parent, child2);

    assert_equals(2, xmpp_stanza_children_length(parent));
    assert_pointer_equals(child1, xmpp_stanza_children(parent));
    assert_pointer_equals(child3, xmpp_stanza_next(child1));

    assert_pointer_equals(parent, xmpp_stanza_parent(child1));
    assert_pointer_equals(NULL, xmpp_stanza_parent(child2));
    assert_pointer_equals(parent, xmpp_stanza_parent(child3));

    xmpp_stanza_del(parent, true);
    xmpp_stanza_del(child2, true);
}

/** Tests converting a stanza to a string. */
void test_string1(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("a", NULL);
    static const char *XML = "<a/>";

    size_t len = 0;
    char *str = xmpp_stanza_string(a, &len);
    assert_string_equals(XML, str);
    assert_equals(strlen(XML), len);

    free(str);
    xmpp_stanza_del(a, true);
}

/** Tests converting a stanza with a namespace and prefix to a string. */
void test_string2(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("uri a foo", NULL);
    static const char *XML = "<foo:a/>";

    size_t len = 0;
    char *str = xmpp_stanza_string(a, &len);
    assert_string_equals(XML, str);
    assert_equals(strlen(XML), len);

    free(str);
    xmpp_stanza_del(a, true);
}

/** Tests converting a stanza with attributes to a string. */
void test_string3(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("a", (const char*[]){
            "foo", "bar",
            "bin", "baz",
            NULL,
    });
    static const char *XML = "<a foo='bar' bin='baz'/>";

    size_t len = 0;
    char *str = xmpp_stanza_string(a, &len);
    assert_equals(strlen(XML), len);

    assert_not_equals(NULL, strstr(str, "<a "));
    assert_not_equals(NULL, strstr(str, "foo='bar'"));
    assert_not_equals(NULL, strstr(str, "bin='baz'"));
    assert_not_equals(NULL, strstr(str, "/>"));

    free(str);
    xmpp_stanza_del(a, true);
}

/** Tests converting a stanza with attributes with other quotes to a string. */
void test_string4(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("a", (const char*[]){
            "foo", "aaa'bbb",
            "bin", "'cc",
            "bar", "dd'",
            "aaa", "fff",
            NULL,
    });
    static const char *XML = "<a foo=\"aaa'bbb\" bin=\"'cc\" bar=\"dd'\" aaa='fff'/>";

    size_t len = 0;
    char *str = xmpp_stanza_string(a, &len);
    assert_equals(strlen(XML), len);

    assert_not_equals(NULL, strstr(str, "<a "));
    assert_not_equals(NULL, strstr(str, "foo=\"aaa'bbb\""));
    assert_not_equals(NULL, strstr(str, "bin=\"'cc\""));
    assert_not_equals(NULL, strstr(str, "bar=\"dd'\""));
    assert_not_equals(NULL, strstr(str, "aaa='fff'"));
    assert_not_equals(NULL, strstr(str, "/>"));

    free(str);
    xmpp_stanza_del(a, true);
}

/** Tests converting a stanza with childeren to a string. */
void test_string5(void) {
    struct xmpp_stanza *parent = xmpp_stanza_new("parent", (const char*[]){
            "foo", "bar",
            NULL,
    });
    struct xmpp_stanza *child = xmpp_stanza_new("child", (const char*[]){
            "bin", "baz",
            NULL,
    });
    xmpp_stanza_append_child(parent, child);

    static const char *XML = "<parent foo='bar'><child bin='baz'/></parent>";

    size_t len = 0;
    char *str = xmpp_stanza_string(parent, &len);
    assert_equals(strlen(XML), len);
    assert_string_equals(XML, str);

    free(str);
    xmpp_stanza_del(parent, true);
}

/** Tests converting a stanza with data to a string. */
void test_string6(void) {
    struct xmpp_stanza *a = xmpp_stanza_new("a", NULL);
    static const char *DATA = "hello world";
    xmpp_stanza_append_data(a, DATA, strlen(DATA));

    static const char *XML = "<a>hello world</a>";

    size_t len = 0;
    char *str = xmpp_stanza_string(a, &len);
    assert_string_equals(XML, str);
    assert_equals(strlen(XML), len);

    free(str);
    xmpp_stanza_del(a, true);
}

/** Tests converting a stanza with childeren and data to a string. */
void test_string7(void) {
    struct xmpp_stanza *parent = xmpp_stanza_new("parent", (const char*[]){
            "foo", "bar",
            NULL,
    });
    struct xmpp_stanza *child = xmpp_stanza_new("child", (const char*[]){
            "bin", "baz",
            NULL,
    });
    xmpp_stanza_append_child(parent, child);
    static const char *DATA = "hello world";
    xmpp_stanza_append_data(parent, DATA, strlen(DATA));

    static const char *XML = "<parent foo='bar'>hello world<child bin='baz'/></parent>";

    size_t len = 0;
    char *str = xmpp_stanza_string(parent, &len);
    assert_equals(strlen(XML), len);
    assert_string_equals(XML, str);

    free(str);
    xmpp_stanza_del(parent, true);
}
