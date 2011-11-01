/**
 * xmp3 - XMPP Proxy
 * xmpp_message.{c,h} - Handles message stanza routing and processing.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include "xmpp_common.h"

typedef bool (*xmpp_message_route)(struct xmpp_stanza *from_stanza, void *data);

void xmpp_message_register_route(struct xmpp_server *server, struct jid *jid,
                                 xmpp_message_route route_func, void *data);

void xmpp_message_deregister_route(struct xmpp_server *server,
                                   struct jid *jid);

void xmpp_message_route(struct xmpp_stanza *stanza);

bool xmpp_message_to_client(struct xmpp_stanza *from_stanza, void *data);
