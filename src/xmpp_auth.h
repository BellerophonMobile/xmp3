/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implements the initial client authentication.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <expat.h>

void xmpp_auth_set_handlers(XML_Parser parser);
