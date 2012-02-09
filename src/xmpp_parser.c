/**
 * xmp3 - XMPP Proxy
 * Copyright (c) 2011 Drexel University
 *
 * @file xmpp_parser.c
 * DOM-style XML parser for XMPP stanzas.
 */

#include <expat.h>

#include "log.h"

#include "xmpp_stanza.h"

#include "xmpp_parser.h"

const char XMPP_PARSER_SEPARATOR = '#';

struct xmpp_parser {
    XML_Parser parser;
    xmpp_parser_handler handler;
    void *data;

    struct xmpp_stanza *cur_stanza;
    int depth;
};

static void stream_start(void *data, const char *name, const char **attrs);
static void start(void *data, const char *name, const char **attrs);
static void chardata(void *data, const char *s, int len);
static void end(void *data, const char *name);

struct xmpp_parser* xmpp_parser_new(bool stream_start) {
    struct xmpp_parser *parser = calloc(1, sizeof(*parser));
    check_mem(parser);

    parser->parser = XML_ParserCreateNS(NULL, XMPP_PARSER_SEPARATOR);
    check(parser->parser != NULL, "Error creating XML parser");

    XML_SetElementHandler(parser->parser, start, end);
    XML_SetCharacterDataHandler(parser->parser, chardata);
    XML_SetUserData(parser->parser, parser);

    if (stream_start) {
        XML_SetStartElementHandler(parser->parser, stream_start);
    }

    return parser;

error:
    free(parser);
    return NULL;
}

void xmpp_parser_del(struct xmpp_parser *parser) {
    XML_ParserFree(parser->parser);
    free(parser);
}

void xmpp_parser_set_handler(struct xmpp_parser *parser,
                             xmpp_parser_handler handler) {
    parser->handler = handler;
}

void xmpp_parser_set_data(struct xmpp_parser *parser, void *data) {
    parser->data = data;
}

bool xmpp_parser_parse(struct xmpp_parser *parser, const char *buf, int len) {
    return XML_Parse(parser->parser, buf, len, 0) == XML_OK;
}

bool xmpp_parser_reset(struct xmppp_parser *parser) {
    XML_ParserReset(parser->parser, NULL) == XML_TRUE;
}

static void stream_start(void *data, const char *name, const char **attrs) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    struct xmpp_stanza *stanza = xmpp_stanza_new(name, attrs);
    parser->handler(stanza, parser, parser->data);
    xmpp_stanza_del(stanza, false);
    XML_SetStartElementHandler(parser->parser, start);
}

static void start(void *data, const char *name, const char **attrs) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;

    struct xmpp_stanza *stanza = xmpp_stanza_new(name, attrs);

    if (depth > 0) {
        xmpp_stanza_append_child(parser->cur_stanza, stanza);
    }

    parser->cur_stanza = stanza;
    parser->depth++;
}

static void chardata(void *data, const char *s, int len) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    xmpp_parser_append_data(parser->cur_stanza, s, len);
}

static void end(void *data, const char *name) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    parser->depth--;

    if (depth < 0) {
        XML_StopParser(parser->parser, false);
    } else if (depth == 0) {
        parser->handler(parser->cur_stanza, parser, parser->data);
        xmpp_stanza_del(parser->cur_stanza, true);
    } else {
        parser->cur_stanza = xmpp_stanza_parent(parser->cur_stanza);
    }
}
