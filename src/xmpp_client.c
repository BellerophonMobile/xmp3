/*
 * Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file xmpp_client.c
 * A connected XMPP server client.
 */

#include "log.h"

#include "client_socket.h"
#include "event.h"
#include "jid.h"
#include "xmpp_auth.h"
#include "xmpp_core.h"
#include "xmpp_parser.h"
#include "xmpp_server.h"

#include "xmpp_client.h"

/** Data on a connected client. */
struct xmpp_client {
    /** The server this client is connected to. */
    struct xmpp_server *server;

    /** The connected client_socket structure. */
    struct client_socket *socket;

    /** An XML parser instance for this client. */
    struct xmpp_parser *parser;

    /** The JID of this client. */
    struct jid *jid;
};

struct xmpp_client* xmpp_client_new(struct xmpp_server *server,
                                    struct client_socket *socket) {
    struct xmpp_client *client = calloc(1, sizeof(*client));
    check_mem(client);

    client->server = server;
    client->socket = socket;

    /* Create the XML parser we'll use to parse stanzas from the client. */
    client->parser = xmpp_parser_new(true);
    check(client->parser != NULL, "Error creating XML parser");

    /* Set initial handlers to begin authentication. */
    xmpp_parser_set_handler(client->parser, xmpp_auth_stream_start);
    xmpp_parser_set_data(client->parser, client);

    return client;

error:
    xmpp_client_del(client);
    return NULL;
}

void xmpp_client_del(struct xmpp_client *client) {
    if (client->jid) {
        xmpp_server_del_stanza_route(client->server, client->jid,
                xmpp_core_route_client, client);
        jid_del(client->jid);
    }

    if (client->socket) {
        event_deregister_callback(xmpp_server_loop(client->server),
                                  client_socket_fd(client->socket));

        client_socket_close(client->socket);
        client_socket_del(client->socket);
    }

    if (client->parser) {
        xmpp_parser_del(client->parser);
    }

    free(client);
}

struct xmpp_server* xmpp_client_server(struct xmpp_client *client) {
    return client->server;
}

struct client_socket* xmpp_client_socket(struct xmpp_client *client) {
    return client->socket;
}

struct xmpp_parser* xmpp_client_parser(struct xmpp_client *client) {
    return client->parser;
}

struct jid* xmpp_client_jid(const struct xmpp_client *client) {
    return client->jid;
}

void xmpp_client_set_jid(struct xmpp_client *client, struct jid *jid) {
    if (client->jid) {
        jid_del(client->jid);
    }
    client->jid = jid;
}
