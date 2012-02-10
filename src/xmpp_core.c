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
#include "xmpp_common.h"
#include "xmpp_parser.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xmpp_core.h"

#if 0
static const char *MSG_ROSTER_PUSH =
    "<iq id='%d' to='%s' type='set'>"
        "<query xmlns='jabber:iq:roster'>"
            "<item jid='%s' subscription='both'/>"
        "</query>"
    "</iq>";

static const char *MSG_PRESENCE = "<presence from='%s' to='%s'/>";

/** Temporary structure for reading a message stanza. */
struct message_tmp {
    struct xmpp_client *to_client;
    struct xmpp_stanza *from_stanza;
};

// Forward declarations
static void message_client_start(void *data, const char *name,
                                 const char **attrs, struct xmp3_xml *parser);
static void message_client_end(void *data, const char *name,
                               struct xmp3_xml *parser);
static void message_client_data(void *data, const char *s, int len,
                                struct xmp3_xml *parser);

static void iq_start(void *data, const char *name, const char **attrs,
                     struct xmp3_xml *parser);

static void presence_start(void *data, const char *name, const char **attrs,
                           struct xmp3_xml *parser);
static void presence_end(void *data, const char *name,
                               struct xmp3_xml *parser);
#endif


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



#if 0
void xmpp_core_stanza_start(void *data, const char *name, const char **attrs,
                            struct xmp3_xml *parser) {
    struct xmpp_server *server = (struct xmpp_server*)data;

    struct xmpp_client *client = xmpp_server_get_cur_client(server);

    struct xmpp_stanza *stanza = xmpp_stanza_new(server, parser, client, name,
                                                 attrs);
    check_mem(stanza);

    if (xmpp_stanza_jid_from(stanza) == NULL) {
        log_err("Stanza without FROM JID, this shouldn't happen.");
    }

    if (xmpp_stanza_jid_to(stanza) == NULL) {
        log_err("Stanza without TO JID, this shouldn't happen.");
    }

    /* We expect any stanza callbacks to change the XML callbacks if
     * neccessary, else the whole stanza is ignored. */
    xmp3_xml_replace_handlers(parser, xmpp_ignore_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, stanza);

    if (!xmpp_server_route_stanza(stanza)) {
        log_warn("Unhandled stanza");
    }
}

void xmpp_core_stanza_end(void *data, const char *name,
                          struct xmp3_xml *parser) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_server *server = xmpp_stanza_server(stanza);

    if (strcmp(name, xmpp_stanza_ns_name(stanza)) != 0) {
        return;
    }

    log_info("Stanza end");

    // Clean up our stanza structure.
    xmpp_stanza_del(stanza);

    // We expect to see the start of a new stanza next.
    xmp3_xml_replace_handlers(parser, xmpp_core_stanza_start,
                              xmpp_core_stream_end, xmpp_ignore_data, server);
}

void xmpp_core_stream_end(void *data, const char *name,
                          struct xmp3_xml *parser) {
    xmp3_xml_stop_parser(parser, false);
}

bool xmpp_core_stanza_handler(struct xmpp_stanza *stanza, void *data) {
    if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_MESSAGE) == 0) {
        log_warn("Message addressed to server?");
    } else if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_PRESENCE) == 0) {
        // TODO: Handle presence stanzas.
        log_info("Presence stanza");
        xmp3_xml_replace_start_handler(xmpp_stanza_parser(stanza),
                                       presence_start);
        return true;
    } else if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_IQ) == 0) {
        log_info("IQ stanza start");
        xmp3_xml_replace_start_handler(xmpp_stanza_parser(stanza), iq_start);
        return true;
    } else {
        log_warn("Unknown stanza");
    }
    return false;
}

bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data) {
    struct xmpp_client *to_client = (struct xmpp_client*)data;

    if (strcmp(xmpp_stanza_ns_name(from_stanza), XMPP_MESSAGE) != 0) {
        return false;
    }

    struct message_tmp *tmp = NULL;
    char* msg = xmpp_stanza_create_tag(from_stanza);
    check_mem(msg);

    check(client_socket_sendall(xmpp_client_socket(to_client),
                                msg, strlen(msg)) > 0,
          "Error sending message start tag to destination.");
    free(msg);

    tmp = calloc(1, sizeof(*tmp));
    check_mem(tmp);

    tmp->to_client = to_client;
    tmp->from_stanza = from_stanza;

    // Set the XML handlers to expect the rest of the message stanza.
    xmp3_xml_replace_handlers(xmpp_stanza_parser(from_stanza),
                              message_client_start, message_client_end,
                              message_client_data, tmp);
    return true;

error:
    free(msg);
    free(tmp);
    xmpp_server_disconnect_client(to_client);
    xmp3_xml_replace_handlers(xmpp_stanza_parser(from_stanza),
                              xmpp_ignore_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, from_stanza);
    return false;
}

/** Handles a starting tag inside of a <message> stanza. */
static void message_client_start(void *data, const char *name,
                                 const char **attrs, struct xmp3_xml *parser) {
    struct message_tmp *tmp = (struct message_tmp*)data;

    struct xmpp_stanza *from_stanza = tmp->from_stanza;

    log_info("Client message start");

    check(client_socket_sendxml(xmpp_client_socket(tmp->to_client), parser)
            > 0, "Error sending message to destination.");
    return;

error:
    xmpp_server_disconnect_client(tmp->to_client);
    free(tmp);
    xmp3_xml_replace_handlers(parser, xmpp_ignore_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, from_stanza);
}

/** Handles an end tag inside of a <message> stanza. */
static void message_client_end(void *data, const char *name,
                               struct xmp3_xml *parser) {
    struct message_tmp *tmp = (struct message_tmp*)data;

    log_info("Client message end");

    /* For self-closing tags "<foo/>" we get an end event without any data to
     * send.  We already sent the end tag with the start tag. */
    if (xmp3_xml_get_byte_count(parser) > 0) {
        check(client_socket_sendxml(xmpp_client_socket(tmp->to_client), parser)
                > 0, "Error sending message to destination.");
    }

    // If we received a </message> tag, we are done parsing this stanza.
    if (strcmp(name, XMPP_MESSAGE) == 0) {
        xmp3_xml_replace_user_data(parser, tmp->from_stanza);
        free(tmp);
        xmpp_core_stanza_end(tmp->from_stanza, name, parser);
    }
    return;

error:
    xmpp_server_disconnect_client(tmp->to_client);
    xmp3_xml_replace_handlers(parser, xmpp_ignore_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, tmp->from_stanza);
    free(tmp);
}

/** Handles character data inside of a <message> stanza. */
static void message_client_data(void *data, const char *s, int len,
                                struct xmp3_xml *parser) {
    struct message_tmp *tmp = (struct message_tmp*)data;

    log_info("Client message data");

    check(client_socket_sendxml(xmpp_client_socket(tmp->to_client), parser)
            > 0, "Error sending message to destination.");
    return;

error:
    xmpp_server_disconnect_client(tmp->to_client);
    xmp3_xml_replace_handlers(parser, xmpp_ignore_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, tmp->from_stanza);
    free(tmp);
}

/** Handles the routing of an IQ stanza. */
static void iq_start(void *data, const char *name, const char **attrs,
                     struct xmp3_xml *parser) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;

    /* Routing for IQ stanzas is based on the namespace (and tag name) of the
     * first child of the top IQ tag. */
    if (!xmpp_server_route_iq(stanza, name)) {
        /* If we can't answer the IQ, then let the client know.  Make sure to
         * ignore anything inside this stanza. */
        xmpp_send_service_unavailable(stanza);
        xmp3_xml_replace_handlers(parser, xmpp_ignore_start,
                                  xmpp_core_stanza_end, xmpp_ignore_data,
                                  data);
    }
}

/** Handles the routing of a presence stanza. */
static void presence_start(void *data, const char *name, const char **attrs,
                     struct xmp3_xml *parser) {
    //struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    xmp3_xml_replace_handlers(parser, xmpp_ignore_start,
                              presence_end, xmpp_ignore_data, data);
}

static void presence_end(void *data, const char *name, struct xmp3_xml *parser)
{
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;

    if (strcmp(name, xmpp_stanza_ns_name(stanza)) != 0) {
        return;
    }

    struct xmpp_server *server = xmpp_stanza_server(stanza);
    char *from_str = jid_to_str(xmpp_stanza_jid_from(stanza));

    /* TODO: For now, just send presence to everyone connected to the server.
     * In the future, we should have a separate module managing each user's
     * roster, etc. */

    struct xmpp_client *local_client = xmpp_server_find_client(server,
            xmpp_stanza_jid_from(stanza));

    struct xmpp_client_iterator *iter = xmpp_client_iterator_new(server);
    struct xmpp_client *client;
    while ((client = xmpp_client_iterator_next(iter)) != NULL) {
        char *to_str = jid_to_str(xmpp_client_jid(client));
        int roster_id = (rand() % 999) + 1;
        char roster_msg[strlen(MSG_ROSTER_PUSH)
                        + strlen(from_str)
                        + strlen(to_str)
                        + 4]; // 3 bytes for 3 digit ID, 1 for null terminator
        sprintf(roster_msg, MSG_ROSTER_PUSH, roster_id, to_str, from_str);
        if (client_socket_sendall(xmpp_client_socket(client), roster_msg,
                                  strlen(roster_msg)) <= 0) {
            log_err("Error sending roster push message to client.");
            xmpp_server_disconnect_client(client);
            free(to_str);
            continue;
        }

        char presence_msg[strlen(MSG_PRESENCE)
                          + strlen(from_str)
                          + strlen(to_str) + 1];
        sprintf(presence_msg, MSG_PRESENCE, from_str, to_str);
        if (client_socket_sendall(xmpp_client_socket(client), presence_msg,
                                  strlen(presence_msg)) <= 0) {
            log_err("Error sending presence message to client.");
            xmpp_server_disconnect_client(client);
            free(to_str);
            continue;
        }

        // If this is a local client, send it presences for everyone connected.
        if (local_client != NULL) {
            char local_presence_msg[strlen(MSG_PRESENCE)
                                    + strlen(from_str)
                                    + strlen(to_str) + 1];
            sprintf(local_presence_msg, MSG_PRESENCE, to_str, from_str);
            if (client_socket_sendall(xmpp_client_socket(local_client),
                                      local_presence_msg,
                                      strlen(local_presence_msg)) <= 0) {
                log_err("Error sending presence message to local client.");
                xmpp_server_disconnect_client(local_client);
                free(to_str);
                continue;
            }
        }
        free(to_str);
    }

    free(from_str);
    xmpp_client_iterator_del(iter);
    xmpp_core_stanza_end(data, name, parser);
}
#endif
