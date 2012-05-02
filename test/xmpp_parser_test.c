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
 * @file xmpp_parser_test.c
 * Unit tests for XML parser functions.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmockery.h>

#include "xmpp_parser.c"

static int called = 0;

void setup(void **state) {
    called = 0;
}

void teardown(void **state) {
    /* Don't need to do any teardown. */
}

static bool cb1(struct xmpp_stanza *stanza, struct xmpp_parser *parser,
                void *data) {
    called++;
    return true;
}

/** Tests that the stanza callback gets called. */
void test_handler1(void **state) {
    static const char *DATA = "<a/>";

    struct xmpp_parser* parser = xmpp_parser_new(false);
    xmpp_parser_set_handler(parser, &cb1);
    assert_true(xmpp_parser_parse(parser, DATA, strlen(DATA)));
    assert_int_equal(called, 1);
}

/** Tests that the stanza callback gets called twice for the stream start. */
void test_handler2(void **state) {
    static const char *DATA = "<stream><a/></stream>";

    struct xmpp_parser* parser = xmpp_parser_new(true);
    xmpp_parser_set_handler(parser, &cb1);
    assert_true(xmpp_parser_parse(parser, DATA, strlen(DATA)));
    assert_int_equal(called, 2);
}

/** Tests that the stanza callback gets called for each of the top-level
 * stanzas. */
void test_handler3(void **state) {
    static const char *DATA = "<stream><a/><b/><c/></stream>";

    struct xmpp_parser* parser = xmpp_parser_new(true);
    xmpp_parser_set_handler(parser, &cb1);
    assert_true(xmpp_parser_parse(parser, DATA, strlen(DATA)));
    assert_int_equal(called, 4);
}

/** Tests that the stanza callback gets called for each of the top-level
 * stanzas. */
void test_handler4(void **state) {
    static const char *DATA = "<stream><a><a><a></a></a></a><b/><c/></stream>";

    struct xmpp_parser* parser = xmpp_parser_new(true);
    xmpp_parser_set_handler(parser, &cb1);
    assert_true(xmpp_parser_parse(parser, DATA, strlen(DATA)));
    assert_int_equal(called, 4);
}

int main(int argc, char *argv[]) {
    const UnitTest tests[] = {
        unit_test_setup_teardown(test_handler1, setup, teardown),
        unit_test_setup_teardown(test_handler2, setup, teardown),
        unit_test_setup_teardown(test_handler3, setup, teardown),
        unit_test_setup_teardown(test_handler4, setup, teardown),
    };
    return run_tests(tests);
}
