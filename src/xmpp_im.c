/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "xmpp_common.h"

// Forward declarations
static void msg_start(void *data, const char *name, const char **attrs);
static void msg_data(void *data, const char *s, int len);
static void msg_end(void *data, const char *name);

void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, msg_start, msg_end);
    XML_SetCharacterDataHandler(parser, msg_data);
}

static void msg_start(void *data, const char *name, const char **attrs) {
    //struct client_info *info = (struct client_info*)data;

    log_info("Message start");
    xmpp_print_start_tag(name, attrs);
}

static void msg_data(void *data, const char *s, int len) {
    //struct client_info *info = (struct client_info*)data;

    log_info("Message data");
    xmpp_print_data(s, len);
}

static void msg_end(void *data, const char *name) {
    //struct client_info *info = (struct client_info*)data;

    log_info("Message end");
    xmpp_print_end_tag(name);
}
