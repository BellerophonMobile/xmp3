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
 * @file xmpp_core.c
 * Handles base stanza routing
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

    /* If an IQ is addressed to a bare JID, it should be handled by the server
     * on behalf of the client. */
    if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_IQ) == 0) {
        const char *to_jid_str = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO);
        struct jid* to_jid = jid_new_from_str(to_jid_str);

        if (jid_resource(to_jid) == NULL) {
            return xmpp_server_route_iq(server, stanza);
        }
    }

    char *strjid = jid_to_str(xmpp_client_jid(client));
    debug("Routing to local client '%s'", strjid);
    free(strjid);

    size_t length;
    char *msg = xmpp_stanza_string(stanza, &length, true);
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
