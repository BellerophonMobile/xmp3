/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "utstring.h"

#include "log.h"

#include "xmpp_server.h"
#include "xmpp_stanza.h"

#include "xmpp_im.h"

const char *XMPP_IQ_SESSION_NS = "urn:ietf:params:xml:ns:xmpp-session";
const char *XMPP_IQ_DISCO_ITEMS_NS = "http://jabber.org/protocol/disco#items";
const char *XMPP_IQ_DISCO_INFO_NS = "http://jabber.org/protocol/disco#info";
const char *XMPP_IQ_ROSTER_NS = "jabber:iq:roster";

static const char *IQ_SESSION = "session";
static const char *IQ_QUERY = "query";

static bool get_roster(struct xmpp_stanza *stanza, struct xmpp_server *server);

bool xmpp_im_iq_session(struct xmpp_stanza *stanza, struct xmpp_server *server,
                        void *data) {
    log_info("Session IQ");

    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_STANZA_TYPE_SET) == 0, "Session IQ type must be 'set'");

    struct xmpp_stanza *child = xmpp_stanza_children(stanza);
    check(strcmp(xmpp_stanza_name(child), IQ_SESSION) == 0,
          "Unexpected stanza.");

    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    struct xmpp_stanza *response = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_FROM, "localhost",
            XMPP_STANZA_ATTR_TO, from,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_RESULT,
            NULL});

    xmpp_server_route_stanza(server, response);
    xmpp_stanza_del(response, true);

    return true;
error:
    return false;
}

bool xmpp_im_iq_disco_items(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data) {
    log_info("Disco Items IQ");

    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_STANZA_TYPE_GET) == 0, "Disco IQ type must be 'get'");

    struct xmpp_stanza *child = xmpp_stanza_children(stanza);
    check(strcmp(xmpp_stanza_name(child), IQ_QUERY) == 0,
          "Unexpected stanza.");

    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    struct xmpp_stanza *response = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_FROM, "localhost",
            XMPP_STANZA_ATTR_TO, from,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_RESULT,
            NULL});

    struct xmpp_stanza *res_query = xmpp_stanza_new("query", (const char*[]){
            "xmlns", XMPP_IQ_DISCO_ITEMS_NS,
            NULL});
    xmpp_stanza_append_child(response, res_query);

    // TODO: Loop over components here

    xmpp_server_route_stanza(server, response);
    xmpp_stanza_del(response, true);

    return true;
error:
    return false;
}

bool xmpp_im_iq_disco_info(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data) {
    log_info("Disco Info IQ");

    check(strcmp(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE),
                 XMPP_STANZA_TYPE_GET) == 0, "Disco IQ type must be 'get'");

    struct xmpp_stanza *child = xmpp_stanza_children(stanza);
    check(strcmp(xmpp_stanza_name(child), IQ_QUERY) == 0,
          "Unexpected stanza.");

    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    struct xmpp_stanza *response = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_FROM, "localhost",
            XMPP_STANZA_ATTR_TO, from,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_RESULT,
            NULL});

    struct xmpp_stanza *res_query = xmpp_stanza_new("query", (const char*[]){
            "xmlns", XMPP_IQ_DISCO_INFO_NS,
            NULL});
    xmpp_stanza_append_child(response, res_query);

    struct xmpp_stanza *tmp = xmpp_stanza_new("identity", (const char*[]){
            "category", "server",
            "type", "im",
            "name", "xmp3",
            NULL});
    xmpp_stanza_append_child(res_query, tmp);

    tmp = xmpp_stanza_new("feature", (const char*[]){
            "var", "http://jabber.org/protocol/disco#info",
            NULL});
    xmpp_stanza_append_child(res_query, tmp);

    tmp = xmpp_stanza_new("feature", (const char*[]){
            "var", "http://jabber.org/protocol/disco#items",
            NULL});
    xmpp_stanza_append_child(res_query, tmp);

    xmpp_server_route_stanza(server, response);
    xmpp_stanza_del(response, true);

    return true;
error:
    return false;
}

bool xmpp_im_iq_roster(struct xmpp_stanza *stanza, struct xmpp_server *server,
                       void *data) {
    log_info("Roster IQ");

    const char *type = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE);

    if (strcmp(type, XMPP_STANZA_TYPE_GET) == 0) {
        return get_roster(stanza, server);
    } else {
        log_warn("Only getting roster IQs is supported for now.");
        return false;
    }
}

static bool get_roster(struct xmpp_stanza *stanza,
                       struct xmpp_server *server) {
    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);

    struct xmpp_stanza *response = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_FROM, "localhost",
            XMPP_STANZA_ATTR_TO, from,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_RESULT,
            NULL});

    struct xmpp_stanza *query = xmpp_stanza_new("query", (const char*[]){
            "xmlns", XMPP_IQ_ROSTER_NS,
            NULL});
    xmpp_stanza_append_child(response, query);

    // TODO: Iterate over user's roster here.

    xmpp_server_route_stanza(server, response);
    xmpp_stanza_del(response, true);
    return true;
}

#if 0
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
#endif
