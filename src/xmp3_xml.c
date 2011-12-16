/**
 * xmp3 - XMPP Proxy
 * xmp3_xml.{c,h} - Functions on top of Expat to support mutliple handlers.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <expat.h>

#include "utlist.h"

#include "log.h"

#include "xmp3_xml.h"
#include "xmpp_common.h"

struct handler {
    xmp3_xml_start_handler start;
    xmp3_xml_end_handler end;
    xmp3_xml_data_handler chardata;
    void *data;

    // Stored in a linked list
    struct handler *prev;
    struct handler *next;
};

struct xmp3_xml {
    XML_Parser parser;

    struct handler *handlers;
    struct handler *cur_handler;
};

// Forward declarations
static void xml_start(void *data, const char *name, const char **attrs);
static void xml_end(void *data, const char *name);
static void xml_data(void *data, const char *s, int len);

struct xmp3_xml* xmp3_xml_new() {
    struct xmp3_xml *parser = calloc(1, sizeof(*parser));
    check_mem(parser);

    parser->parser = XML_ParserCreateNS(NULL, *XMPP_NS_SEPARATOR);
    check(parser->parser != NULL, "Error creating XML parser");

    XML_SetElementHandler(parser->parser, xml_start, xml_end);
    XML_SetCharacterDataHandler(parser->parser, xml_data);
    XML_SetUserData(parser->parser, parser);

    return parser;

error:
    free(parser);
    return NULL;
}

void xmp3_xml_del(struct xmp3_xml *parser) {
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(parser->handlers, handler, handler_tmp) {
        free(handler);
    }
    free(parser);
}

enum XML_Status xmp3_xml_parse(struct xmp3_xml *parser, const char *s, int len,
                               int isFinal) {
    return XML_Parse(parser->parser, s, len, isFinal);
}

enum XML_Status xmp3_xml_stop_parser(struct xmp3_xml *parser, bool resumable) {
    return XML_StopParser(parser->parser, resumable);
}

int xmp3_xml_get_byte_count(struct xmp3_xml *parser) {
    return XML_GetCurrentByteCount(parser->parser);
}

const char* xmp3_xml_get_buffer(struct xmp3_xml *parser, int *offset,
                                int *size) {
    return XML_GetInputContext(parser->parser, offset, size);
}

const char* xmp3_xml_get_error_str(struct xmp3_xml *parser) {
    return XML_ErrorString(XML_GetErrorCode(parser->parser));
}

void xmp3_xml_add_handlers(struct xmp3_xml *parser,
                           xmp3_xml_start_handler start,
                           xmp3_xml_end_handler end,
                           xmp3_xml_data_handler chardata,
                           void *data) {
    struct handler *handler = calloc(1, sizeof(*handler));
    check_mem(handler);
    handler->start = start;
    handler->end = end;
    handler->chardata = chardata;
    handler->data = data;
    DL_PREPEND(parser->handlers, handler);
}

void* xmp3_xml_get_current_user_data(struct xmp3_xml *parser) {
    if (parser->cur_handler == NULL) {
        return NULL;
    }
    return parser->cur_handler->data;
}

void xmp3_xml_replace_handlers(struct xmp3_xml *parser,
                               xmp3_xml_start_handler start,
                               xmp3_xml_end_handler end,
                               xmp3_xml_data_handler chardata,
                               void *data) {
    xmp3_xml_replace_start_handler(parser, start);
    xmp3_xml_replace_end_handler(parser, end);
    xmp3_xml_replace_chardata_handler(parser, chardata);
    xmp3_xml_replace_user_data(parser, data);
}

void xmp3_xml_replace_start_handler(struct xmp3_xml *parser,
                                    xmp3_xml_start_handler start) {
    if (parser->cur_handler == NULL) {
        return;
    }
    parser->cur_handler->start = start;
}

void xmp3_xml_replace_end_handler(struct xmp3_xml *parser,
                                  xmp3_xml_end_handler end) {
    if (parser->cur_handler == NULL) {
        return;
    }
    parser->cur_handler->end = end;
}

void xmp3_xml_replace_chardata_handler(struct xmp3_xml *parser,
                                       xmp3_xml_data_handler chardata) {
    if (parser->cur_handler == NULL) {
        return;
    }
    parser->cur_handler->chardata = chardata;
}

void xmp3_xml_replace_user_data(struct xmp3_xml *parser, void *data) {
    if (parser->cur_handler == NULL) {
        return;
    }
    parser->cur_handler->data = data;
}

void* xmp3_xml_get_user_data(struct xmp3_xml *parser) {
    if (parser->cur_handler == NULL) {
        return NULL;
    }
    return parser->cur_handler->data;
}

void xmp3_xml_remove_handlers(struct xmp3_xml *parser) {
    if (parser->cur_handler == NULL) {
        return;
    }
    DL_DELETE(parser->handlers, parser->cur_handler);
    free(parser->cur_handler);
    parser->cur_handler = NULL;
}

static void xml_start(void *data, const char *name, const char **attrs) {
    struct xmp3_xml *parser = (struct xmp3_xml*)data;
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(parser->handlers, handler, handler_tmp) {
        parser->cur_handler = handler;
        handler->start(handler->data, name, attrs, parser);
    }
    parser->cur_handler = NULL;
}

static void xml_end(void *data, const char *name) {
    struct xmp3_xml *parser = (struct xmp3_xml*)data;
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(parser->handlers, handler, handler_tmp) {
        parser->cur_handler = handler;
        handler->end(handler->data, name, parser);
    }
    parser->cur_handler = NULL;
}

static void xml_data(void *data, const char *s, int len) {
    struct xmp3_xml *parser = (struct xmp3_xml*)data;
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(parser->handlers, handler, handler_tmp) {
        parser->cur_handler = handler;
        handler->chardata(handler->data, s, len, parser);
    }
    parser->cur_handler = NULL;
}
