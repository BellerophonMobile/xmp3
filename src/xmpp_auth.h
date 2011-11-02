/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implement initial XMPP authendication
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <expat.h>

void xmpp_auth_set_handlers(XML_Parser parser);
