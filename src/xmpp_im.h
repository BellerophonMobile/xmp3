/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <expat.h>

#include "xmpp_common.h"

struct iq_data {
    struct client_info *info;
    struct common_attrs attrs;
    XML_EndElementHandler end_handler;
};

void xmpp_im_set_handlers(XML_Parser parser);
