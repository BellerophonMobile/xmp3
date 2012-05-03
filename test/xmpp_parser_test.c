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

struct test_data {
    struct xmpp_parser *parser;
    int called;
};

static bool cb1(struct xmpp_stanza *stanza, struct xmpp_parser *parser,
                void *state) {
    struct test_data *data = state;
    debug("adsf");
    data->called++;
    return true;
}

struct test_data *test_data_new(bool is_stream_start) {
    struct test_data *data = calloc(1, sizeof(*data));
    check_mem(data);

    data->called = 0;
    data->parser = xmpp_parser_new(is_stream_start);
    xmpp_parser_set_handler(data->parser, &cb1);
    xmpp_parser_set_data(data->parser, data);

    return data;
}

void setup(void **state) {
    *state = (void*)test_data_new(false);
}

void setup_stream_start(void **state) {
    *state = (void*)test_data_new(true);
}

void teardown(void **state) {
    struct test_data *data = *state;
    xmpp_parser_del(data->parser);
}

/** Tests that the stanza callback gets called. */
void test_handler1(void **state) {
    static const char *XML = "<a/>";
    struct test_data *data = *state;
    assert_true(xmpp_parser_parse(data->parser, XML, strlen(XML)));
    assert_int_equal(data->called, 1);
}

/** Tests that the stanza callback gets called twice for the stream start. */
void test_handler2(void **state) {
    struct test_data *data = *state;
    static const char *XML = "<stream><a/></stream>";
    assert_true(xmpp_parser_parse(data->parser, XML, strlen(XML)));
    assert_int_equal(data->called, 2);
}

/** Tests that the stanza callback gets called for each of the top-level
 * stanzas. */
void test_handler3(void **state) {
    struct test_data *data = *state;
    static const char *XML = "<stream><a/><b/><c/></stream>";
    assert_true(xmpp_parser_parse(data->parser, XML, strlen(XML)));
    assert_int_equal(data->called, 4);
}

/** Tests that the stanza callback gets called for each of the top-level
 * stanzas. */
void test_handler4(void **state) {
    struct test_data *data = *state;
    static const char *XML = "<stream><a><a><a></a></a></a><b/><c/></stream>";
    assert_true(xmpp_parser_parse(data->parser, XML, strlen(XML)));
    assert_int_equal(data->called, 4);
}

int main(int argc, char *argv[]) {
    const UnitTest tests[] = {
        unit_test_setup_teardown(test_handler1, setup, teardown),
        unit_test_setup_teardown(test_handler2, setup_stream_start, teardown),
        unit_test_setup_teardown(test_handler3, setup_stream_start, teardown),
        unit_test_setup_teardown(test_handler4, setup_stream_start, teardown),
    };
    return run_tests(tests);
}
