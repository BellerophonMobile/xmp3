/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <expat.h>

#include "utstring.h"

#include "log.h"
#include "utils.h"

#include "client_socket.h"
#include "jid.h"
#include "xmp3_xml.h"
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

static const char *MSG_ROSTER_START =
    "<iq id='%s' type='result'>"
        "<query xmlns='jabber:iq:roster'>";

static const char *MSG_ROSTER_ITEM =
            "<item jid='%s' subscription='both'/>";

static const char *MSG_ROSTER_END =
        "</query>"
    "</iq>";

static const char *MSG_DISCO_QUERY_INFO =
    "<iq id='%s' type='result' from='localhost'>"
        "<query xmlns='http://jabber.org/protocol/disco#info'>"
            "<identity category='server' type='im' name='XMP3'/>"
            "<feature var='http://jabber.org/protocol/disco#info'/>"
            "<feature var='http://jabber.org/protocol/disco#items'/>"
        "</query>"
    "</iq>";

static const char *MSG_DISCO_QUERY_ITEMS =
    "<iq id='%s' type='result' from='localhost'>"
        "<query xmlns='http://jabber.org/protocol/disco#items'>"
            "<item jid='conference.localhost' name='Public Chatrooms'/>"
        "</query>"
    "</iq>";

// Forward declarations
static void session_end(void *data, const char *name,
                        struct xmp3_xml *parser);
static void roster_query_end(void *data, const char *name,
                             struct xmp3_xml *parser);
static void disco_query_info_end(void *data, const char *name,
                                 struct xmp3_xml *parser);
static void disco_query_items_end(void *data, const char *name,
                                  struct xmp3_xml *parser);

bool xmpp_im_iq_session(struct xmpp_stanza *stanza, void *data) {
    log_info("Session IQ Start");

    check(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID) != NULL,
          "Session IQ needs id attribute");
    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    // We expect to see the </session> tag next
    xmp3_xml_replace_handlers(xmpp_stanza_parser(stanza), xmpp_error_start,
                              session_end, xmpp_ignore_data, stanza);
    return true;

error:
    return false;
}

/** Handles the end </session> tag in a session IQ. */
static void session_end(void *data, const char *name,
                        struct xmp3_xml *parser) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_server_find_client(
            xmpp_stanza_server(stanza), xmpp_stanza_jid_from(stanza));

    log_info("Session IQ End");
    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");

    if (client != NULL) {
        char msg[strlen(MSG_STREAM_SUCCESS)
                 + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];
        sprintf(msg, MSG_STREAM_SUCCESS,
                xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
        check(client_socket_sendall(xmpp_client_socket(client), msg,
                                    strlen(msg)) > 0,
              "Error sending stream success message");
    }

    // We expect to see the </iq> tag next
    xmp3_xml_replace_handlers(parser, xmpp_error_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, data);
    return;

error:
    xmp3_xml_stop_parser(parser, false);
}

bool xmpp_im_iq_roster_query(struct xmpp_stanza *stanza, void *data) {

    log_info("Roster Query IQ Start");

    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_ATTR_TYPE_GET) == 0,
          "Session IQ type must be \"set\"");

    // We expect to see the </query> tag next.
    xmp3_xml_replace_handlers(xmpp_stanza_parser(stanza), xmpp_error_start,
                              roster_query_end, xmpp_ignore_data, stanza);
    return true;

error:
    return false;
}

/** Handles the end </query> in a roster query. */
static void roster_query_end(void *data, const char *name,
                             struct xmp3_xml *parser) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_server *server = xmpp_stanza_server(stanza);
    struct xmpp_client *client = xmpp_server_find_client(server,
            xmpp_stanza_jid_from(stanza));

    log_info("Roster Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");

    if (client != NULL) {
        /* TODO: Actually manage the user's roster.  Instead, everyone
         * currently connected is in the user's roster. */

        UT_string msg;
        utstring_init(&msg);
        utstring_printf(&msg, MSG_ROSTER_START,
                        xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));

        struct xmpp_client_iterator *iter = xmpp_client_iterator_new(server);
        struct xmpp_client *from_client;
        while ((from_client = xmpp_client_iterator_next(iter)) != NULL) {
            char *jid = jid_to_str(xmpp_client_jid(from_client));
            utstring_printf(&msg, MSG_ROSTER_ITEM, jid);
            free(jid);
        }
        xmpp_client_iterator_del(iter);

        utstring_printf(&msg, MSG_ROSTER_END);

        ssize_t rv = client_socket_sendall(xmpp_client_socket(client),
                                           utstring_body(&msg),
                                           utstring_len(&msg));
        utstring_done(&msg);
        check(rv > 0, "Error sending roster message");
    }

    // We expect to see the </iq> tag next.
    xmp3_xml_replace_handlers(parser, xmpp_error_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, data);
    return;

error:
    xmp3_xml_stop_parser(parser, false);
}

bool xmpp_im_iq_disco_query_info(struct xmpp_stanza *stanza, void *data) {
    log_info("Info Query IQ Start");
    xmp3_xml_replace_end_handler(xmpp_stanza_parser(stanza),
                                 disco_query_info_end);
    return true;
}

static void disco_query_info_end(void *data, const char *name,
                                 struct xmp3_xml *parser) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_server_find_client(
            xmpp_stanza_server(stanza), xmpp_stanza_jid_from(stanza));

    log_info("Info Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_INFO) == 0, "Unexpected stanza");

    if (client != NULL) {
        char msg[strlen(MSG_DISCO_QUERY_INFO)
                 + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];
        sprintf(msg, MSG_DISCO_QUERY_INFO,
                xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
        check(client_socket_sendall(xmpp_client_socket(client), msg,
                                    strlen(msg)) > 0,
              "Error sending info query IQ message");
    }

    xmp3_xml_replace_handlers(parser, xmpp_error_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, data);
    return;

error:
    xmp3_xml_stop_parser(parser, false);
}

bool xmpp_im_iq_disco_query_items(struct xmpp_stanza *stanza, void *data) {
    log_info("Items Query IQ Start");
    xmp3_xml_replace_end_handler(xmpp_stanza_parser(stanza),
                                 disco_query_items_end);
    return true;
}

static void disco_query_items_end(void *data, const char *name,
                                  struct xmp3_xml *parser) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = xmpp_server_find_client(
            xmpp_stanza_server(stanza), xmpp_stanza_jid_from(stanza));

    log_info("Items Query IQ End");
    check(strcmp(name, XMPP_IQ_DISCO_QUERY_ITEMS) == 0, "Unexpected stanza");

    if (client != NULL) {
        char msg[strlen(MSG_DISCO_QUERY_ITEMS)
                 + strlen(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID))];
        sprintf(msg, MSG_DISCO_QUERY_ITEMS,
                xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID));
        check(client_socket_sendall(xmpp_client_socket(client), msg,
                                    strlen(msg)) > 0,
              "Error sending items query IQ message");
    }

    xmp3_xml_replace_handlers(parser, xmpp_error_start, xmpp_core_stanza_end,
                              xmpp_ignore_data, data);
    return;

error:
    xmp3_xml_stop_parser(parser, false);
}
