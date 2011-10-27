/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <expat.h>

void xmpp_im_set_handlers(XML_Parser parser);

void xmpp_im_stanza_end(void *data, const char *name);
