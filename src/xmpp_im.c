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
    // Hard code MUC for now...

    struct xmpp_stanza *item = xmpp_stanza_new("item", (const char*[]){
            "jid", "conference.localhost",
            "name", "Public Chatrooms",
            NULL});
    xmpp_stanza_append_child(res_query, item);

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
