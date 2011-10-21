/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Implement RFC6121 XMPP-CORE functions.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <expat.h>

void xmpp_core_set_handlers(XML_Parser parser);
