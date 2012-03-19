/**
 * xmp3 - XMPP Proxy
 * Copyright (c) 2011 Drexel University
 *
 * @file xmpp_parser.c
 * DOM-style XML parser for XMPP stanzas.
 */

#include <expat.h>

#include <utlist.h>

#include "log.h"
#include "utils.h"
#include "xmpp_stanza.h"
#include "xmpp_parser.h"

const char XMPP_PARSER_SEPARATOR = ' ';

/** Structure representing an XML namespace. */
struct xmpp_parser_namespace {
    char *prefix;
    char *uri;

    struct xmpp_parser_namespace *next;
};

/** Structure holding state for the XML parser. */
struct xmpp_parser {
    XML_Parser parser;
    xmpp_parser_handler handler;
    void *data;

    struct xmpp_parser_namespace *namespaces;
    struct xmpp_stanza *cur_stanza;
    int depth;
};

static void stream_start(void *data, const char *name, const char **attrs);
static void start(void *data, const char *name, const char **attrs);
static void chardata(void *data, const char *s, int len);
static void end(void *data, const char *name);
static void ns_start(void *data, const char *prefix, const char *uri);

struct xmpp_parser* xmpp_parser_new(bool is_stream_start) {
    struct xmpp_parser *parser = calloc(1, sizeof(*parser));
    check_mem(parser);

    parser->parser = XML_ParserCreateNS(NULL, XMPP_PARSER_SEPARATOR);
    check(parser->parser != NULL, "Error creating XML parser");

    XML_SetReturnNSTriplet(parser->parser, true);

    XML_SetElementHandler(parser->parser, start, end);
    XML_SetCharacterDataHandler(parser->parser, chardata);
    XML_SetUserData(parser->parser, parser);
    XML_SetStartNamespaceDeclHandler(parser->parser, ns_start);

    if (is_stream_start) {
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

const char* xmpp_parser_strerror(struct xmpp_parser *parser) {
    return XML_ErrorString(XML_GetErrorCode(parser->parser));
}

void xmpp_parser_set_handler(struct xmpp_parser *parser,
                             xmpp_parser_handler handler) {
    parser->handler = handler;
}

void xmpp_parser_set_data(struct xmpp_parser *parser, void *data) {
    parser->data = data;
}

bool xmpp_parser_parse(struct xmpp_parser *parser, const char *buf, int len) {
    return XML_Parse(parser->parser, buf, len, 0) == XML_STATUS_OK;
}

bool xmpp_parser_reset(struct xmpp_parser *parser, bool is_stream_start) {
    if (XML_ParserReset(parser->parser, NULL) != XML_TRUE) {
        return false;
    }

    XML_SetElementHandler(parser->parser, start, end);
    XML_SetCharacterDataHandler(parser->parser, chardata);
    XML_SetUserData(parser->parser, parser);

    if (is_stream_start) {
        XML_SetStartElementHandler(parser->parser, stream_start);
    }
    return true;
}

void xmpp_parser_new_stream(struct xmpp_parser *parser) {
    XML_SetStartElementHandler(parser->parser, stream_start);
}

const char* xmpp_parser_namespace_uri(struct xmpp_parser_namespace *ns) {
    return ns->uri;
}

const char* xmpp_parser_namespace_prefix(struct xmpp_parser_namespace *ns) {
    return ns->prefix;
}

struct xmpp_parser_namespace* xmpp_parser_namespace_next(
        struct xmpp_parser_namespace *ns) {
    return ns->next;
}

void xmpp_parser_namespace_del(struct xmpp_parser_namespace *ns) {
    struct xmpp_parser_namespace *tmp;
    while (ns != NULL) {
        tmp = ns->next;
        if (ns->prefix) {
            free(ns->prefix);
        }
        free(ns->uri);
        free(ns);
        ns = tmp;
    }
}

/** Expat callback for the start of an XMPP stream. */
static void stream_start(void *data, const char *ns_name, const char **attrs) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    struct xmpp_stanza *stanza = xmpp_stanza_ns_new(ns_name, attrs,
                                                    parser->namespaces);

    char *str = xmpp_stanza_string(stanza, NULL);
    log_info("Received stanza: %s", str);
    free(str);

    parser->namespaces = NULL;
    if (!parser->handler(stanza, parser, parser->data)) {
        XML_StopParser(parser->parser, false);
    } else {
        parser->depth = 0;
        XML_SetStartElementHandler(parser->parser, start);
    }
    xmpp_stanza_del(stanza, false);
}

/** Expat callback for the start of any other element. */
static void start(void *data, const char *ns_name, const char **attrs) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    struct xmpp_stanza *stanza = xmpp_stanza_ns_new(ns_name, attrs,
                                                    parser->namespaces);

    parser->namespaces = NULL;
    if (parser->depth > 0) {
        xmpp_stanza_append_child(parser->cur_stanza, stanza);
    }
    parser->cur_stanza = stanza;
    parser->depth++;
}

/** Expat callback for XML data. */
static void chardata(void *data, const char *s, int len) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    if (parser->cur_stanza) {
        xmpp_stanza_append_data(parser->cur_stanza, s, len);
    }
}

/** Expat callback for end elements. */
static void end(void *data, const char *name) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;
    parser->depth--;

    if (parser->depth < 0) {
        XML_StopParser(parser->parser, false);
    } else if (parser->depth == 0) {
        char *stanza_str = xmpp_stanza_string(parser->cur_stanza, NULL);
        debug("Received stanza: %s", stanza_str);
        free(stanza_str);

        if (!parser->handler(parser->cur_stanza, parser, parser->data)) {
            XML_StopParser(parser->parser, false);
        }
        xmpp_stanza_del(parser->cur_stanza, true);
        parser->cur_stanza = NULL;
    } else {
        parser->cur_stanza = xmpp_stanza_parent(parser->cur_stanza);
    }
}

/** Expat callback for namespace declarations. */
static void ns_start(void *data, const char *prefix, const char *uri) {
    struct xmpp_parser *parser = (struct xmpp_parser*)data;

    struct xmpp_parser_namespace *ns = calloc(1, sizeof(*ns));
    check_mem(ns);

    if (prefix) {
        STRDUP_CHECK(ns->prefix, prefix);
    }
    STRDUP_CHECK(ns->uri, uri);
    LL_PREPEND(parser->namespaces, ns);
}
