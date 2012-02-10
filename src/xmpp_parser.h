/**
 * xmp3 - XMPP Proxy
 * Copyright (c) 2011 Drexel University
 *
 * @file xmpp_parser.h
 * DOM-style XML parser for XMPP stanzas.
 */

#pragma once

#include <stdbool.h>

struct xmpp_parser;
struct xmpp_stanza;

extern const char XMPP_PARSER_SEPARATOR;

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

bool xmpp_parser_reset(struct xmpp_parser *parser, bool is_stream_start);

void xmpp_parser_new_stream(struct xmpp_parser *parser);
