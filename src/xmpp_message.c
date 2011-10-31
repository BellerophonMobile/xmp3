/**
 * xmp3 - XMPP Proxy
 * xmpp_message.{c,h} - Handles message stanza routing and processing.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_message.h"

#include "xmpp.h"
#include "xmpp_common.h"

void xmpp_message_route(struct xmpp_stanza *stanza) {
    struct xmpp_server *server = stanza->from->server;
    struct message_route *route = xmpp_find_message_route(server, &stanza->to);
    if (route == NULL) {
        log_info("No route for destination");
        return false;
    }
    return route->route_func(stanza, route->data);
}
