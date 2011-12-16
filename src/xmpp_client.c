/**
 * xmp3 - XMPP Proxy
 * xmpp_client.{c,h} - A connected XMPP server client.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "log.h"

#include "client_socket.h"
#include "event.h"
#include "jid.h"
#include "xmp3_xml.h"
#include "xmpp_auth.h"
#include "xmpp_common.h"
#include "xmpp_core.h"
#include "xmpp_server.h"

#include "xmpp_client.h"

/** Data on a connected client. */
struct xmpp_client {
    /** The server this client is connected to. */
    struct xmpp_server *server;

    /** The connected client_socket structure. */
    struct client_socket *socket;

    /** An XML parser instance for this client. */
    struct xmp3_xml *parser;

    /** The JID of this client. */
    struct jid *jid;
};

struct xmpp_client* xmpp_client_new(struct xmpp_server *server,
                                    struct client_socket *socket) {
    struct xmpp_client *client = calloc(1, sizeof(*client));
    check_mem(client);

    client->server = server;
    client->socket = socket;

    // Create the XML parser we'll use to parse stanzas from the client.
    client->parser = xmp3_xml_new();
    check(client->parser != NULL, "Error creating XML parser");

    // Set initial handlers to begin authentication.
    xmp3_xml_add_handlers(client->parser, xmpp_auth_stream_start,
                          xmpp_error_end, xmpp_error_data, client);

    return client;

error:
    xmpp_client_del(client);
    return NULL;
}

void xmpp_client_del(struct xmpp_client *client) {
    if (client->jid) {
        xmpp_server_del_stanza_route(client->server, client->jid,
                xmpp_core_message_handler, client);
        jid_del(client->jid);
    }

    if (client->socket) {
        event_deregister_callback(xmpp_server_loop(client->server),
                                  client_socket_fd(client->socket));

        client_socket_close(client->socket);
        client_socket_del(client->socket);
    }

    if (client->parser) {
        xmp3_xml_del(client->parser);
    }

    free(client);
}

struct xmpp_server* xmpp_client_server(struct xmpp_client *client) {
    return client->server;
}

struct client_socket* xmpp_client_socket(struct xmpp_client *client) {
    return client->socket;
}

struct xmp3_xml* xmpp_client_parser(struct xmpp_client *client) {
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
