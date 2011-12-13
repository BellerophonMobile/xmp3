/**
 * xmp3 - XMPP Proxy
 * xmp3_xml.{c,h} - Functions on top of Expat to support mutliple handlers.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "utlist.h"

#include "log.h"

#include "xmp3_xml.h"

struct handler {
    XML_StartElementHandler start;
    XML_EndElementHandler end;
    XML_CharacterDataHandler chardata;
    void *data;

    // Stored in a linked list
    struct handler *prev;
    struct handler *next;
};

struct xml_handlers {
    XML_Parser parser;

    struct handler *handlers;
    struct handler *cur_handler;
};

// Forward declarations
static void xml_start(void *data, const char *name, const char **attrs);
static void xml_end(void *data, const char *name);
static void xml_data(void *data, const char *s, int len);

void xmp3_xml_init_parser(XML_Parser parser) {
    struct xml_handlers *handlers = calloc(1, sizeof(*handlers));
    check_mem(handlers);

    XML_SetElementHandler(parser, xml_start, xml_end);
    XML_SetCharacterDataHandler(parser, xml_data);
    XML_SetUserData(parser, handlers);
}

void xmp3_xml_add_handlers(XML_Parser parser,
                           XML_StartElementHandler start,
                           XML_EndElementHandler end,
                           XML_CharacterDataHandler chardata,
                           void *data) {
    struct xml_handlers *handlers = XML_GetUserData(parser);

    struct handler *handler = calloc(1, sizeof(*handler));
    check_mem(handler);
    handler->start = start;
    handler->end = end;
    handler->chardata = chardata;
    handler->data = data;
    DL_PREPEND(handlers->handlers, handler);
}

void* xmp3_xml_get_current_user_data(XML_Parser parser) {
    struct xml_handlers *handlers = XML_GetUserData(parser);
    if (handlers->cur_handler == NULL) {
        return NULL;
    }
    return handlers->cur_handler->data;
}

void xmp3_xml_replace_handlers(XML_Parser parser,
                               XML_StartElementHandler start,
                               XML_EndElementHandler end,
                               XML_CharacterDataHandler chardata,
                               void *data) {
    xmp3_xml_replace_start_handler(parser, start);
    xmp3_xml_replace_end_handler(parser, end);
    xmp3_xml_replace_chardata_handler(parser, chardata);
    xmp3_xml_replace_user_data(parser, data);
}

void xmp3_xml_replace_start_handler(XML_Parser parser,
                                    XML_StartElementHandler start) {
    struct xml_handlers *handlers = XML_GetUserData(parser);
    if (handlers->cur_handler == NULL) {
        return;
    }
    handlers->cur_handler->start = start;
}

void xmp3_xml_replace_end_handler(XML_Parser parser,
                                  XML_EndElementHandler end) {
    struct xml_handlers *handlers = XML_GetUserData(parser);
    if (handlers->cur_handler == NULL) {
        return;
    }
    handlers->cur_handler->end = end;
}

void xmp3_xml_replace_chardata_handler(XML_Parser parser,
                                       XML_CharacterDataHandler chardata) {
    struct xml_handlers *handlers = XML_GetUserData(parser);
    if (handlers->cur_handler == NULL) {
        return;
    }
    handlers->cur_handler->chardata = chardata;
}

void xmp3_xml_replace_user_data(XML_Parser parser, void *data) {
    struct xml_handlers *handlers = XML_GetUserData(parser);
    if (handlers->cur_handler == NULL) {
        return;
    }
    handlers->cur_handler->data = data;
}

void xmp3_xml_remove_handler(XML_Parser parser) {
    struct xml_handlers *handlers = XML_GetUserData(parser);
    if (handlers->cur_handler == NULL) {
        return;
    }
    DL_DELETE(handlers->handlers, handlers->cur_handler);
    free(handlers->cur_handler);
    handlers->cur_handler = NULL;
}

static void xml_start(void *data, const char *name, const char **attrs) {
    struct xml_handlers *handlers = (struct xml_handlers*)data;
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(handlers->handlers, handler, handler_tmp) {
        handlers->cur_handler = handler;
        handler->start(handler->data, name, attrs);
    }
    handlers->cur_handler = NULL;
}

static void xml_end(void *data, const char *name) {
    struct xml_handlers *handlers = (struct xml_handlers*)data;
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(handlers->handlers, handler, handler_tmp) {
        handlers->cur_handler = handler;
        handler->end(handler->data, name);
    }
    handlers->cur_handler = NULL;
}

static void xml_data(void *data, const char *s, int len) {
    struct xml_handlers *handlers = (struct xml_handlers*)data;
    struct handler *handler, *handler_tmp;
    DL_FOREACH_SAFE(handlers->handlers, handler, handler_tmp) {
        handlers->cur_handler = handler;
        handler->chardata(handler->data, s, len);
    }
    handlers->cur_handler = NULL;
}
