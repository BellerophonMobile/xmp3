/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "utstring.h"

#include "log.h"
#include "utils.h"
#include "xmpp.h"
#include "xmpp_common.h"
#include "xep_disco.h"

// XML String constants
static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

static const char *MSG_ROSTER =
    "<iq id='%s' type='result'>"
        "<query xmlns='jabber:iq:roster'>"
        "</query>"
    "</iq>";

static const char *MSG_MESSAGE_START = "<message";

// Forward declarations
static void stanza_start(void *data, const char *name, const char **attrs);

static void message_client_start(void *data, const char *name,
                                 const char **attrs);
static void message_client_end(void *data, const char *name);
static void message_client_data(void *data, const char *s, int len);

static void handle_presence(struct xmpp_stanza *stanza, const char **attrs);
static void presence_start(void *data, const char *name, const char **attrs);
static void presence_end(void *data, const char *name);

static void handle_iq(struct xmpp_stanza *stanza, const char **attrs);
static void iq_start(void *data, const char *name, const char **attrs);
static void iq_end(void *data, const char *name);


static bool iq_session(struct xmpp_stanza *stanza, const char *name,
                       const char **attrs);
static void iq_session_end(void *data, const char *name);

static bool iq_roster_query(struct xmpp_stanza *stanza, const char *name,
                            const char **attrs);
static void iq_roster_query_end(void *data, const char *name);

static struct xep_iq_namespace {
    const char *namespace;
    xmpp_stanza_handler handler;
} IQ_NAMESPACES[] = {
    {XMPP_NS_SESSION, iq_session},
    {XMPP_NS_ROSTER, iq_roster_query},
    {XMPP_NS_DISCO_ITEMS, disco_query_items},
    {XMPP_NS_DISCO_INFO, disco_query_info},
};

void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stanza_start, xmpp_stream_end);
    XML_SetCharacterDataHandler(parser, xmpp_ignore_data);
}

bool xmpp_im_message_to_client(struct xmpp_stanza *from_stanza, void *data) {
    struct xmpp_client *to_client = (struct xmpp_client*)data;
    struct xmpp_client *from_client = from_stanza->from;
    XML_SetElementHandler(from_client->parser, message_client_start,
                          message_client_end);
    XML_SetCharacterDataHandler(from_client->parser, message_client_data);

    /* We need to add a "from" field to the outgoing stanza regardless of
     * whether or not the client gave us one. */
    UT_string *msg;
    utstring_new(msg);

    utstring_printf(msg, MSG_MESSAGE_START);

    UT_string *from = jid_to_str(&from_stanza->from->jid);
    utstring_printf(msg, " %s='%s'", XMPP_ATTR_FROM, utstring_body(from));
    utstring_free(from);

    UT_string *to = jid_to_str(&from_stanza->to);
    utstring_printf(msg, " %s='%s'", XMPP_ATTR_TO, utstring_body(to));
    utstring_free(to);

    for (int i = 0; from_stanza->other_attrs[i] != NULL; i += 2) {
        utstring_printf(msg, " %s='%s'", from_stanza->other_attrs[i],
                        from_stanza->other_attrs[i + 1]);
    }

    if (sendall(to_client->fd, utstring_body(msg), utstring_len(msg)) <= 0) {
        log_err("Error sending message start tag to destination.");
    }

    utstring_free(msg);
    return true;
}

void xmpp_im_stanza_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from;

    log_info("Stanza end");
    XML_SetUserData(client->parser, client);
    xmpp_im_set_handlers(client->parser);
    del_stanza(stanza);
}

static void stanza_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Stanza start");

    struct xmpp_stanza *stanza = new_stanza(client, name, attrs);
    XML_SetUserData(client->parser, stanza);
    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        xmpp_route_message(stanza);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        handle_presence(stanza, attrs);
    } else if (strcmp(name, XMPP_IQ) == 0) {
        handle_iq(stanza, attrs);
    } else {
        log_warn("Unknown stanza");
    }
}

static void message_client_start(void *data, const char *name,
                                 const char **attrs) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner message start");
    //route_message(stanza);
}

static void message_client_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner message end");
    //route_message(stanza);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        xmpp_im_stanza_end(data, name);
    }
    return;
}

static void message_client_data(void *data, const char *s, int len) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    log_info("Inner message data");
    xmpp_print_data(s, len);
    //route_message(stanza);
}

static void handle_presence(struct xmpp_stanza *stanza,
                            const char **attrs) {
    struct xmpp_client *client = stanza->from;
    XML_SetElementHandler(client->parser, presence_start, presence_end);
}

static void presence_start(void *data, const char *name, const char **attrs) {
    log_info("Inner presence start");
    // ignore
}

static void presence_end(void *data, const char *name) {
    //struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner presence end");

    if (strcmp(name, XMPP_PRESENCE) == 0) {
        xmpp_im_stanza_end(data, name);
    }
}

static void handle_iq(struct xmpp_stanza *stanza,
                      const char **attrs) {
    struct xmpp_client *client = stanza->from;
    XML_SetElementHandler(client->parser, iq_start, iq_end);
}

static void iq_start(void *data, const char *name, const char **attrs) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner IQ start");

    if (stanza->is_unhandled) {
        /* We are not going to magically start handling an IQ block
         * mid-parse. */
        return;
    }

    int i;
    const int end = sizeof(IQ_NAMESPACES) / sizeof(struct xep_iq_namespace);
    for (i = 0; i < end; i++) {
        /* name is "namespace tag_name", a space inbetween.  We only want to
         * compare namespaces. */
        int ns_len = strchr(name, ' ') - name;
        if (strncmp(name, IQ_NAMESPACES[i].namespace, ns_len) == 0) {
            if (IQ_NAMESPACES[i].handler(stanza, name, attrs)) {
                return;
            }
        }
    }

    // If we get here, we haven't handled the stanza
    log_warn("Unhandled iq start");
    stanza->is_unhandled = true;
}

static void iq_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner IQ end");

    if (strcmp(name, XMPP_IQ) == 0) {
        if (stanza->is_unhandled && stanza->id != NULL) {
            xmpp_send_not_supported(stanza);
        }
        xmpp_im_stanza_end(data, name);
    }
}

static bool iq_session(struct xmpp_stanza *stanza, const char *name,
                       const char **attrs) {
    struct xmpp_client *client = stanza->from;

    if (strcmp(stanza->name, XMPP_IQ) != 0) {
        return false;
    }

    log_info("Session IQ Start");

    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");
    check(stanza->id != NULL, "Session IQ needs id attribute");
    check(strcmp(stanza->type, XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    XML_SetEndElementHandler(client->parser, iq_session_end);
    return true;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client->parser, false);
    return false;
}

static void iq_session_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from;

    char msg[strlen(MSG_STREAM_SUCCESS) + strlen(stanza->id)];

    log_info("Session IQ End");

    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_STREAM_SUCCESS, stanza->id);
    check(sendall(client->fd, msg, strlen(msg)) > 0,
          "Error sending stream success message");

    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client->parser, false);
}

static bool iq_roster_query(struct xmpp_stanza *stanza, const char *name,
                            const char **attrs) {
    struct xmpp_client *client = stanza->from;

    if (strcmp(stanza->name, XMPP_IQ) != 0) {
        return false;
    }

    log_info("Roster Query IQ Start");

    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");
    check(strcmp(stanza->type, XMPP_ATTR_TYPE_GET) == 0,
          "Session IQ type must be \"set\"");

    XML_SetEndElementHandler(client->parser, iq_roster_query_end);
    return true;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client->parser, false);
    return false;
}

static void iq_roster_query_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from;

    char msg[strlen(MSG_ROSTER) + strlen(stanza->id)];

    log_info("Roster Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_ROSTER, stanza->id);
    check(sendall(client->fd, msg, strlen(msg)) > 0,
          "Error sending roster message");

    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client->parser, false);
}
