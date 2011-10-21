/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "log.h"
#include "xmpp_common.h"
#include "xmpp_im_iq.h"

// Forward declarations
static void msg_start(void *data, const char *name, const char **attrs);

// Presence messages
static void handle_presence(struct client_info *info, const char **attrs);

// Messages
static void handle_message(struct client_info *info, const char **attrs);

void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, msg_start, xmpp_error_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void msg_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Message start");
    xmpp_print_start_tag(name, attrs);

    if (strcmp(name, XMPP_IQ) == 0) {
        xmpp_im_iq_handle(info, attrs);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        handle_presence(info, attrs);
    } else if (strcmp(name, XMPP_MESSAGE) == 0) {
        handle_message(info, attrs);
    } else {
        sentinel("Unexpected XMPP message type.");
    }
    return;

error:
    XML_StopParser(info->parser, false);
}

static void handle_presence(struct client_info *info, const char **attrs) {
    log_info("Got Presence msg");
}

static void handle_message(struct client_info *info, const char **attrs) {
    log_info("Got Message");
}
