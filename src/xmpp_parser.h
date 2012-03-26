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
 * @file xmpp_parser.h
 * DOM-style XML parser for XMPP stanzas.
 */

#pragma once

#include <stdbool.h>

struct xmpp_parser;
struct xmpp_parser_namespace;
struct xmpp_stanza;

extern const char XMPP_PARSER_SEPARATOR;

/** Callback to be notified of new XMPP stanza. */
typedef bool (*xmpp_parser_handler)(struct xmpp_stanza *stanza,
                                    struct xmpp_parser *parser,
                                    void *data);

/**
 * @param stream_start Whether the first start tag is expected to be a stream
 *                     start.  When true, the callback will be called initially
 *                     with a childless stanza representing the stream header.
 */
struct xmpp_parser* xmpp_parser_new(bool is_stream_start);

void xmpp_parser_del(struct xmpp_parser *parser);

const char* xmpp_parser_strerror(struct xmpp_parser *parser);

void xmpp_parser_set_handler(struct xmpp_parser *parser,
                             xmpp_parser_handler handler);

void xmpp_parser_set_data(struct xmpp_parser *parser, void *data);

bool xmpp_parser_parse(struct xmpp_parser *parser, const char *buf, int len);

/** Reset the state of the parser as if it was just created. */
bool xmpp_parser_reset(struct xmpp_parser *parser);

void xmpp_parser_new_stream(struct xmpp_parser *parser);

const char* xmpp_parser_namespace_uri(struct xmpp_parser_namespace *ns);

const char* xmpp_parser_namespace_prefix(struct xmpp_parser_namespace *ns);

/** Return the next namespace in the namespace list. */
struct xmpp_parser_namespace* xmpp_parser_namespace_next(
        struct xmpp_parser_namespace *ns);

void xmpp_parser_namespace_del(struct xmpp_parser_namespace *ns);
