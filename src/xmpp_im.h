/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <expat.h>

#include "xmpp_common.h"

void xmpp_im_set_handlers(XML_Parser parser);
