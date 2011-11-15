/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xmpp_im.h"

#include <expat.h>

#include "log.h"
#include "utils.h"

#include "xmpp.h"
#include "xmpp_common.h"
#include "xmpp_core.h"

const char *XMPP_IQ_SESSION = XMPP_NS_SESSION XMPP_NS_SEPARATOR "session";
const char *XMPP_IQ_QUERY_ROSTER = XMPP_NS_ROSTER XMPP_NS_SEPARATOR "query";
const char *XMPP_IQ_DISCO_QUERY_ITEMS =
    XMPP_NS_DISCO_ITEMS XMPP_NS_SEPARATOR "query";
const char *XMPP_IQ_DISCO_QUERY_INFO =
    XMPP_NS_DISCO_INFO XMPP_NS_SEPARATOR "query";

// XML String constants
static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

static const char *MSG_ROSTER =
    "<iq id='%s' type='result'>"
        "<query xmlns='jabber:iq:roster'>"
        "</query>"
    "</iq>";

// Forward declarations
static void session_end(void *data, const char *name);
static void roster_query_end(void *data, const char *name);
/*
static void disco_query_info_end(void *data, const char *name);
static void disco_query_items_end(void *data, const char *name);
*/

bool xmpp_im_iq_session(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = stanza->from_client;

    log_info("Session IQ Start");

    check(stanza->id != NULL, "Session IQ needs id attribute");
    check(strcmp(stanza->type, XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    // We expect to see the </session> tag next
    XML_SetElementHandler(client->parser, xmpp_error_start, session_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);
    return true;

error:
    return false;
}

/** Handles the end </session> tag in a session IQ. */
static void session_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;

    char msg[strlen(MSG_STREAM_SUCCESS) + strlen(stanza->id)];

    log_info("Session IQ End");

    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_STREAM_SUCCESS, stanza->id);
    check(sendall(client->fd, msg, strlen(msg)) > 0,
          "Error sending stream success message");

    // We expect to see the </iq> tag next
    XML_SetElementHandler(client->parser, xmpp_error_start,
                          xmpp_core_stanza_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);
    return;

error:
    // TODO: If the client has been disconnected, we should probably remove it.
    return;
}

bool xmpp_im_iq_roster_query(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = stanza->from_client;

    log_info("Roster Query IQ Start");

    check(strcmp(stanza->type, XMPP_ATTR_TYPE_GET) == 0,
          "Session IQ type must be \"set\"");

    // We expect to see the </query> tag next.
    XML_SetElementHandler(client->parser, xmpp_error_start, roster_query_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);
    return true;

error:
    return false;
}

/** Handles the end </query> in a roster query. */
static void roster_query_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;

    char msg[strlen(MSG_ROSTER) + strlen(stanza->id)];

    log_info("Roster Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");

    // TODO: Actually manage the user's roster.
    snprintf(msg, sizeof(msg), MSG_ROSTER, stanza->id);
    check(sendall(client->fd, msg, strlen(msg)) > 0,
          "Error sending roster message");

    // We expect to see the </iq> tag next.
    XML_SetElementHandler(client->parser, xmpp_error_start,
                          xmpp_core_stanza_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client->parser, false);
}

/*
bool xmpp_im_iq_disco_query_items(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = stanza->from_client;

    log_info("Items Query IQ Start");

    XML_SetEndElementHandler(client->parser, disco_query_items_end);
    return true;
}

static void disco_query_items_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;
    xmpp_send_service_unavailable(stanza);
    XML_SetEndElementHandler(client->parser, xmpp_core_stanza_end);
    return;
}

bool xmpp_im_iq_disco_query_info(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = stanza->from_client;

    log_info("Info Query IQ Start");

    XML_SetEndElementHandler(client->parser, disco_query_info_end);
    return true;
}

static void disco_query_info_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;
    xmpp_send_service_unavailable(stanza);
    XML_SetEndElementHandler(client->parser, xmpp_core_stanza_end);
    return;
}
*/
