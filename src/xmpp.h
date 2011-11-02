/**
 * xmp3 - XMPP Proxy
 * xmpp.{c,h} - Implements the server part of a normal XMPP server.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "event.h"
#include "xmpp_common.h"

typedef bool (*xmpp_message_route_callback)(struct xmpp_stanza *from_stanza,
                                            void *data);

struct xmpp_server;

/** Initializes the XMPP server and listens for new connections */
bool xmpp_init(struct event_loop *loop, struct in_addr addr, uint16_t port);

void xmpp_message_register_route(struct xmpp_server *server, struct jid *jid,
                                 xmpp_message_route_callback route_func,
                                 void *data);

void xmpp_message_deregister_route(struct xmpp_server *server,
                                   struct jid *jid);

bool xmpp_message_route(struct xmpp_stanza *stanza);
