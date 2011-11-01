/**
 * xmp3 - XMPP Proxy
 * xmpp.{c,h} - Implements the server part of a normal XMPP server.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include <expat.h>

#include "event.h"
#include "xmpp_common.h"

/** Initializes the XMPP server and listens for new connections */
bool xmpp_init(struct event_loop *loop, struct in_addr addr, uint16_t port);
