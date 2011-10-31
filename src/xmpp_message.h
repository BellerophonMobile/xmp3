/**
 * xmp3 - XMPP Proxy
 * xmpp_message.{c,h} - Handles message stanza routing and processing.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include "xmpp_common.h"

void xmpp_message_route(struct xmpp_stanza *stanza);
