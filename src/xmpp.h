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

typedef bool (*xmpp_message_callback)(struct xmpp_stanza *from_stanza,
                                      void *data);
typedef bool (*xmpp_presence_callback)(struct xmpp_stanza *from_stanza,
                                       void *data);
typedef bool (*xmpp_iq_callback)(struct xmpp_stanza *from_stanza, void *data);

struct xmpp_server;

/** Initializes the XMPP server and listens for new connections */
struct xmpp_server* xmpp_init(struct event_loop *loop, struct in_addr addr,
                              uint16_t port);

void xmpp_register_message_route(struct xmpp_server *server, struct jid *jid,
                                 xmpp_message_callback cb, void *data);

void xmpp_deregister_message_route(struct xmpp_server *server,
                                   struct jid *jid);

bool xmpp_route_message(struct xmpp_stanza *stanza);

void xmpp_register_iq_namespace(struct xmpp_server *server, const char *ns,
                                xmpp_iq_callback cb, void *data);

void xmpp_deregister_iq_namespace(struct xmpp_server *server, const char *ns);

bool xmpp_route_iq(const char *ns, struct xmpp_stanza *stanza);
