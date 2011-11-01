/**
 * xmp3 - XMPP Proxy
 * xmpp_message.{c,h} - Handles message stanza routing and processing.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_message.h"

#include "xmpp.h"
#include "xmpp_common.h"

// Forward declarations
static struct message_route* new_message_route(struct jid *jid,
        xmpp_message_route route_func, void *data);
static void del_message_route(struct message_route *route) {
static struct message_route* find_message_route(struct xmpp_server *server,
                                                struct jid *jid);

void xmpp_message_register_route(struct xmpp_server *server, struct jid *jid,
                                 xmpp_message_route route_func, void *data) {
    struct message_route *route = find_message_route(server, jid);
    if (route != NULL) {
        log_warn("Attempted to insert duplicate route");
        return;
    }
    DL_APPEND(server->message_routes,
              new_message_route(jid, route_func, data));
}

void xmpp_message_deregister_route(struct xmpp_server *server,
                                   struct jid *jid) {
    struct message_route *route = find_message_route(server, jid);
    if (route == NULL) {
        log_warn("Attempted to remove non-existent key");
        return;
    }
    DL_DELETE(server->message_routes, route);
    del_message_route(route);
}

void xmpp_message_route(struct xmpp_stanza *stanza) {
    struct xmpp_server *server = stanza->from->server;
    struct message_route *route = find_message_route(server, &stanza->to);
    if (route == NULL) {
        log_info("No route for destination");
        return false;
    }
    return route->route_func(stanza, route->data);
}

bool xmpp_message_to_client(struct xmpp_stanza *from_stanza, void *data) {
    struct xmpp_client *to_client = (struct xmpp_client*)data;
    struct xmpp_client *from_client = from_stanza->from;
    XML_SetElementHandler(from_client->parser, message_client_start,
                          message_client_end);
    XML_SetCharacterDataHandler(from_client->parser, message_client_data);

    /* We need to add a "from" field to the outgoing stanza regardless of
     * whether or not the client gave us one. */
    UT_string *msg;
    utstring_new(msg);

    utstring_printf(msg, MSG_MESSAGE_START);

    UT_string *from = jid_to_str(&from_stanza->from->jid);
    utstring_printf(msg, " %s='%s'", XMPP_ATTR_FROM, utstring_body(from));
    utstring_free(from);

    UT_string *to = jid_to_str(&from_stanza->to);
    utstring_printf(msg, " %s='%s'", XMPP_ATTR_TO, utstring_body(to));
    utstring_free(to);

    for (int i = 0; from_stanza->other_attrs[i] != NULL; i += 2) {
        utstring_printf(msg, " %s='%s'", from_stanza->other_attrs[i],
                        from_stanza->other_attrs[i + 1]);
    }

    if (sendall(to_client->fd, utstring_body(msg), utstring_len(msg)) <= 0) {
        log_err("Error sending message start tag to destination.");
    }

    utstring_free(msg);
    return true;
}

static struct message_route* new_message_route(struct jid *jid,
        xmpp_message_route route_func, void *data) {
    struct message_route *route = calloc(1, sizeof(*route));
    check_mem(route);
    route->jid = jid;
    route->route_func = route_func;
    route->data = data;
    return route;
}

static void del_message_route(struct message_route *route) {
    free(route);
}

static struct message_route* find_message_route(struct xmpp_server *server,
                                                struct jid *jid) {
    struct message_route *route;
    DL_FOREACH(server->message_routes, route) {
        if (strcmp(route->jid->local, jid->local) == 0
            && strcmp(route->jid->domain, jid->domain) == 0) {
            /* If no resource specified in search, then return the first one
             * that matches the local and domain parts, else return an exact
             * match. */
            if (jid->resource == NULL
                || strcmp(route->jid->resource, jid->resource) == 0) {
                return route;
            }
        }
    }
    return NULL;
}
