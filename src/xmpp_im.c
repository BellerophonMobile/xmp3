/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "log.h"
#include "xmpp_common.h"

// Forward declarations
static void msg_start(void *data, const char *name, const char **attrs);


void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, msg_start, xmpp_error_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void msg_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Message start");
    xmpp_print_start_tag(name, attrs);


}


}
