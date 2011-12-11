/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <expat.h>

#include "utstring.h"

#include "log.h"

#include "jid.h"

#include "client_socket.h"
#include "xmpp_client.h"
#include "xmpp_common.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xmpp_core.h"

/** Temporary structure for reading a message stanza. */
struct message_tmp {
    struct xmpp_client *to_client;
    struct xmpp_stanza *from_stanza;
};

// Forward declarations
static void message_client_start(void *data, const char *name,
                                 const char **attrs);
static void message_client_end(void *data, const char *name);
static void message_client_data(void *data, const char *s, int len);

static void iq_start(void *data, const char *name, const char **attrs);

void xmpp_core_stanza_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    struct xmpp_stanza *stanza = xmpp_stanza_new(client, name, attrs);
    check_mem(stanza);
    XML_SetUserData(xmpp_client_parser(client), stanza);

    /* We expect any stanza callbacks to change the XML callbacks if
     * neccessary, else the whole stanza is ignored. */
    XML_SetElementHandler(xmpp_client_parser(client), xmpp_ignore_start,
                          xmpp_core_stanza_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(client), xmpp_ignore_data);

    if (!xmpp_server_route_stanza(stanza)) {
        log_warn("Unhandled stanza");
    }
}

void xmpp_core_stanza_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    if (strcmp(name, xmpp_stanza_ns_name(stanza)) != 0) {
        return;
    }

    log_info("Stanza end");

    // Clean up our stanza structure.
    XML_SetUserData(xmpp_client_parser(client), client);
    xmpp_stanza_del(stanza);

    // We expect to see the start of a new stanza next.
    XML_SetElementHandler(xmpp_client_parser(client), xmpp_core_stanza_start,
                          xmpp_core_stream_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(client), xmpp_ignore_data);
}

void xmpp_core_stream_end(void *data, const char *name) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    XML_StopParser(xmpp_client_parser(client), false);
}

bool xmpp_core_stanza_handler(struct xmpp_stanza *stanza, void *data) {
    if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_MESSAGE) == 0) {
        log_warn("Message addressed to server?");
    } else if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_PRESENCE) == 0) {
        // TODO: Handle presence stanzas.
        log_info("Presence stanza");
        //handle_presence(stanza, attrs);
        return false;
    } else if (strcmp(xmpp_stanza_ns_name(stanza), XMPP_IQ) == 0) {
        log_info("IQ stanza start");
        XML_SetStartElementHandler(
                xmpp_client_parser(xmpp_stanza_client(stanza)), iq_start);
        return true;
    } else {
        log_warn("Unknown stanza");
    }
    return false;
}

bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data) {
    struct xmpp_client *to_client = (struct xmpp_client*)data;
    struct xmpp_client *from_client = xmpp_stanza_client(from_stanza);

    if (strcmp(xmpp_stanza_ns_name(from_stanza), XMPP_MESSAGE) != 0) {
        return false;
    }

    // Set the XML handlers to expect the rest of the message stanza.
    XML_SetElementHandler(xmpp_client_parser(from_client),
                          message_client_start, message_client_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(from_client), message_client_data);

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
    XML_SetUserData(xmpp_client_parser(from_client), tmp);
    return true;

error:
    free(msg);
    free(tmp);
    xmpp_server_disconnect_client(to_client);
    XML_SetElementHandler(xmpp_client_parser(from_client), xmpp_ignore_start,
                          xmpp_core_stanza_end);
    return false;
}

/** Handles a starting tag inside of a <message> stanza. */
static void message_client_start(void *data, const char *name,
                                 const char **attrs) {
    struct message_tmp *tmp = (struct message_tmp*)data;

    struct xmpp_stanza *from_stanza = tmp->from_stanza;
    struct xmpp_client *from_client = xmpp_stanza_client(from_stanza);

    log_info("Client message start");

    check(client_socket_sendxml(xmpp_client_socket(tmp->to_client),
                xmpp_client_parser(from_client)) > 0,
            "Error sending message to destination.");
    return;

error:
    XML_SetUserData(xmpp_client_parser(from_client), from_stanza);
    xmpp_server_disconnect_client(tmp->to_client);
    XML_SetElementHandler(xmpp_client_parser(from_client), xmpp_ignore_start,
                          xmpp_core_stanza_end);
    free(tmp);
}

/** Handles an end tag inside of a <message> stanza. */
static void message_client_end(void *data, const char *name) {
    struct message_tmp *tmp = (struct message_tmp*)data;
    struct xmpp_client *from_client = xmpp_stanza_client(tmp->from_stanza);

    log_info("Client message end");

    /* For self-closing tags "<foo/>" we get an end event without any data to
     * send.  We already sent the end tag with the start tag. */
    if (XML_GetCurrentByteCount(xmpp_client_parser(from_client)) > 0) {
        check(client_socket_sendxml(
                    xmpp_client_socket(tmp->to_client),
                    xmpp_client_parser(from_client)) > 0,
              "Error sending message to destination.");
    }

    // If we received a </message> tag, we are done parsing this stanza.
    if (strcmp(name, XMPP_MESSAGE) == 0) {
        XML_SetUserData(xmpp_client_parser(from_client), tmp->from_stanza);
        free(tmp);
        xmpp_core_stanza_end(tmp->from_stanza, name);
    }
    return;

error:
    XML_SetUserData(xmpp_client_parser(from_client), tmp->from_stanza);
    xmpp_server_disconnect_client(tmp->to_client);
    XML_SetElementHandler(xmpp_client_parser(from_client), xmpp_ignore_start,
                          xmpp_core_stanza_end);
    free(tmp);
}

/** Handles character data inside of a <message> stanza. */
static void message_client_data(void *data, const char *s, int len) {
    struct message_tmp *tmp = (struct message_tmp*)data;
    struct xmpp_client *from_client = xmpp_stanza_client(tmp->from_stanza);

    log_info("Client message data");

    check(client_socket_sendxml(xmpp_client_socket(tmp->to_client),
                xmpp_client_parser(from_client)) > 0,
            "Error sending message to destination.");
    return;

error:
    XML_SetUserData(xmpp_client_parser(from_client), tmp->from_stanza);
    xmpp_server_disconnect_client(tmp->to_client);
    XML_SetElementHandler(xmpp_client_parser(from_client), xmpp_ignore_start,
                          xmpp_core_stanza_end);
    free(tmp);
}

/** Handles the routing of an IQ stanza. */
static void iq_start(void *data, const char *name, const char **attrs) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    /* Routing for IQ stanzas is based on the namespace (and tag name) of the
     * first child of the top IQ tag. */
    if (!xmpp_server_route_iq(stanza, name)) {
        /* If we can't answer the IQ, then let the client know.  Make sure to
         * ignore anything inside this stanza. */
        xmpp_send_service_unavailable(stanza);
        XML_SetElementHandler(xmpp_client_parser(client), xmpp_ignore_start,
                              xmpp_core_stanza_end);
        XML_SetCharacterDataHandler(xmpp_client_parser(client),
                                    xmpp_ignore_data);
    }
}
