/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <stdlib.h>

#include <utstring.h>

#include "client_socket.h"
#include "jid.h"
#include "log.h"
#include "xmpp_client.h"
#include "xmpp_parser.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xmpp_core.h"

bool xmpp_core_handle_stanza(struct xmpp_stanza *stanza,
                             struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    struct xmpp_server *server = xmpp_client_server(client);

    const char *to = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO);
    if (to == NULL) {
        /* RFC6120 Section 10, messages with no "to" are addressed to the bare
         * JID of the client, other stanzas are addressed to the server. */
        char *new_to;
        if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_MESSAGE) == 0) {
            struct jid *bare = jid_new_from_jid_bare(xmpp_client_jid(client));
            new_to = jid_to_str(bare);
            jid_del(bare);
        } else {
            new_to = jid_to_str(xmpp_server_jid(server));
        }
        xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TO, new_to);
    }

    /* RFC6120 Section 8.1.2.1, the server essentially ignores any "from"
     * attribute and adds the from attribute of the full JID of the client. */
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM,
                         jid_to_str(xmpp_client_jid(client)));

    xmpp_server_route_stanza(server, stanza);

    return true;
}

bool xmpp_core_route_client(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    size_t length;
    char *msg = xmpp_stanza_string(stanza, &length);
    if (client_socket_sendall(xmpp_client_socket(client), msg, length) <= 0) {
        free(msg);
        xmpp_server_disconnect_client(client);
        return false;
    }
    free(msg);
    return true;
}

bool xmpp_core_route_server(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data) {
    if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_MESSAGE) == 0) {
        log_warn("Message addressed to server?");
        return false;
    } else if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_PRESENCE) == 0) {
        log_warn("Ignoring presence stanza.");
        return true;
    } else if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_IQ) == 0) {
        return xmpp_server_route_iq(server, stanza);
    } else {
        log_warn("Unknown stanza");
        return false;
    }
}
