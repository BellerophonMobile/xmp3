/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <expat.h>

#include "log.h"
#include "utils.h"

#include "client_socket.h"
#include "xmpp_client.h"
#include "xmpp_common.h"
#include "xmpp_core.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xmpp_im.h"

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

static const char *MSG_DISCO_QUERY_INFO =
    "<iq id='%s' type='result'>"
        "<query xmlns='http://jabber.org/protocol/disco#info'>"
            "<feature var='http://jabber.org/protocol/disco#info'/>"
            "<feature var='http://jabber.org/protocol/disco#items'/>"
        "</query>"
    "</iq>";

static const char *MSG_DISCO_QUERY_ITEMS =
    "<iq id='%s' type='result'>"
        "<query xmlns='http://jabber.org/protocol/disco#items'>"
        "</query>"
    "</iq>";

// Forward declarations
static void session_end(void *data, const char *name);
static void roster_query_end(void *data, const char *name);
static void disco_query_info_end(void *data, const char *name);
static void disco_query_items_end(void *data, const char *name);

bool xmpp_im_iq_session(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    log_info("Session IQ Start");

    check(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID) != NULL,
          "Session IQ needs id attribute");
    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    // We expect to see the </session> tag next
    XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                          session_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(client), xmpp_error_data);
    return true;

error:
    return false;
}

/** Handles the end </session> tag in a session IQ. */
static void session_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    char msg[strlen(MSG_STREAM_SUCCESS)
             + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];

    log_info("Session IQ End");

    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");

    sprintf(msg, MSG_STREAM_SUCCESS,
            xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
    check(client_socket_sendall(xmpp_client_socket(client), msg,
                                strlen(msg)) > 0,
          "Error sending stream success message");

    // We expect to see the </iq> tag next
    XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                          xmpp_core_stanza_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(client), xmpp_error_data);
    return;

error:
    XML_StopParser(xmpp_client_parser(client), false);
}

bool xmpp_im_iq_roster_query(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    log_info("Roster Query IQ Start");

    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_ATTR_TYPE_GET) == 0,
          "Session IQ type must be \"set\"");

    // We expect to see the </query> tag next.
    XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                          roster_query_end);
    XML_SetCharacterDataHandler(xmpp_client_parser(client), xmpp_error_data);
    return true;

error:
    return false;
}

/** Handles the end </query> in a roster query. */
static void roster_query_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    char msg[strlen(MSG_ROSTER)
             + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];

    log_info("Roster Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");

    // TODO: Actually manage the user's roster.

    sprintf(msg, MSG_ROSTER, xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
    check(client_socket_sendall(xmpp_client_socket(client), msg,
                                strlen(msg)) > 0,
          "Error sending roster message");

    // We expect to see the </iq> tag next.
    XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                          xmpp_core_stanza_end);
    return;

error:
    XML_StopParser(xmpp_client_parser(client), false);
}

bool xmpp_im_iq_disco_query_info(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = xmpp_stanza_client(stanza);
    log_info("Info Query IQ Start");
    XML_SetEndElementHandler(xmpp_client_parser(client), disco_query_info_end);
    return true;
}

static void disco_query_info_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    char msg[strlen(MSG_DISCO_QUERY_INFO)
             + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];

    log_info("Info Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_INFO) == 0, "Unexpected stanza");

    sprintf(msg, MSG_DISCO_QUERY_INFO,
            xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
    check(client_socket_sendall(xmpp_client_socket(client), msg,
                                strlen(msg)) > 0,
          "Error sending info query IQ message");

    XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                          xmpp_core_stanza_end);
    return;

error:
    XML_StopParser(xmpp_client_parser(client), false);
}

bool xmpp_im_iq_disco_query_items(struct xmpp_stanza *stanza, void *data) {
    struct xmpp_client *client = xmpp_stanza_client(stanza);
    log_info("Items Query IQ Start");
    XML_SetEndElementHandler(xmpp_client_parser(client), disco_query_items_end);
    return true;
}

static void disco_query_items_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_stanza_client(stanza);

    char msg[strlen(MSG_DISCO_QUERY_ITEMS)
             + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];

    log_info("Items Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_ITEMS) == 0, "Unexpected stanza");

    sprintf(msg, MSG_DISCO_QUERY_ITEMS,
            xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
    check(client_socket_sendall(xmpp_client_socket(client), msg,
                                strlen(msg)) > 0,
          "Error sending items query IQ message");

    XML_SetElementHandler(xmpp_client_parser(client), xmpp_error_start,
                          xmpp_core_stanza_end);
    return;

error:
    XML_StopParser(xmpp_client_parser(client), false);
}
