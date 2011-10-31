/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "log.h"
#include "utils.h"
#include "xmpp.h"
#include "xmpp_common.h"
#include "xep_disco.h"

/* This should probably go in the utils module, if I use it outside of here,
 * I will. */
#define ALLOC_COPY_STRING(a, b) a = calloc(strlen(b) + 1, sizeof(char)); \
                                check_mem(a); \
                                strcpy(a, b)

// XML String constants
static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

static const char *MSG_ROSTER =
    "<iq id='%s' type='result'>"
        "<query xmlns='jabber:iq:roster'>"
        "</query>"
    "</iq>";

// Forward declarations
static void stanza_start(void *data, const char *name, const char **attrs);

static void handle_message(struct xmpp_stanza *stanza, const char **attrs);
static bool route_message(struct xmpp_stanza *stanza);
static void message_start(void *data, const char *name, const char **attrs);
static void message_end(void *data, const char *name);
static void message_data(void *data, const char *s, int len);

static void handle_presence(struct xmpp_stanza *stanza, const char **attrs);
static void presence_start(void *data, const char *name, const char **attrs);
static void presence_end(void *data, const char *name);

static void handle_iq(struct xmpp_stanza *stanza, const char **attrs);
static void iq_start(void *data, const char *name, const char **attrs);
static void iq_end(void *data, const char *name);


static void new_stanza_jid(struct jid *jid, const char *strjid);

static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                           const char *name,
                                           const char **attrs);

static void del_stanza(struct xmpp_stanza *stanza);

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

static void stanza_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Stanza start");

    struct xmpp_stanza *stanza = new_stanza(client, name, attrs);
    XML_SetUserData(client->parser, stanza);
    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        handle_message(stanza, attrs);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        handle_presence(stanza, attrs);
    } else if (strcmp(name, XMPP_IQ) == 0) {
        handle_iq(stanza, attrs);
    } else {
        log_warn("Unknown stanza");
    }
}

void xmpp_im_stanza_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from;

    log_info("Stanza end");
    XML_SetUserData(client->parser, client);
    xmpp_im_set_handlers(client->parser);
    del_stanza(stanza);
}

static void handle_message(struct xmpp_stanza *stanza,
                           const char **attrs) {
    struct xmpp_client *client = stanza->from;
    XML_SetElementHandler(client->parser, message_start, message_end);
    XML_SetCharacterDataHandler(client->parser, message_data);

    log_info("Message start");
    route_message(stanza);
}

static bool route_message(struct xmpp_stanza *stanza) {
    struct xmpp_client *client = stanza->from;

    struct xmpp_client *to_client = xmpp_find_client(
            &stanza->to, client->server);
    if (to_client != NULL) {
        if (sendxml(client->parser, to_client->fd) <= 0) {
            log_warn("Unable to send message to destination");
            return false;
        }
    } else {
        log_info("Destination not connected.");
        return false;
    }
    return true;
}

static void message_start(void *data, const char *name, const char **attrs) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner message start");
    route_message(stanza);
}

static void message_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Inner message end");
    route_message(stanza);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        xmpp_im_stanza_end(data, name);
    }
    return;
}

static void message_data(void *data, const char *s, int len) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    log_info("Inner message data");
    xmpp_print_data(s, len);
    route_message(stanza);
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

static void new_stanza_jid(struct jid *jid, const char *strjid) {
    char *domainstr = strchr(strjid, '@');
    char *resourcestr = strchr(strjid, '/');

    int locallen;
    if (domainstr == NULL) {
        locallen = strlen(strjid);
    } else {
        locallen = domainstr - strjid;
    }

    jid->local = calloc(1, locallen);
    check_mem(jid->local);
    strncpy(jid->local, strjid, locallen);

    if (domainstr == NULL) {
        return;
    } else {
        domainstr++; // was previously on the '@' character in the string
    }

    int domainlen;
    if (resourcestr == NULL) {
        domainlen = strlen(domainstr);
    } else {
        domainlen = resourcestr - domainstr;
    }
    jid->domain = calloc(1, domainlen);
    check_mem(jid->domain);
    strncpy(jid->domain, domainstr, domainlen);

    if (resourcestr == NULL) {
        return;
    } else {
        resourcestr++; // was previously on the '/' character
    }

    jid->resource = calloc(1, strlen(resourcestr));
    check_mem(jid->resource);
    strncpy(jid->resource, resourcestr, strlen(resourcestr));
}

static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                           const char *name,
                                           const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);
    stanza->is_unhandled = false;
    stanza->from = client;

    ALLOC_COPY_STRING(stanza->name, name);
    for (int i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_ID) == 0) {
            ALLOC_COPY_STRING(stanza->id, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TO) == 0) {
            new_stanza_jid(&stanza->to, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            ALLOC_COPY_STRING(stanza->type, attrs[i + 1]);
        }
    }

    /* RFC6120 Section 8.1.2.1 states that the server is to basically ignore
     * any "from" attribute in the stanza, and append the JID of the client the
     * server got the stanza from. */

    return stanza;
}

static void del_stanza(struct xmpp_stanza *stanza) {
    free(stanza->name);
    free(stanza->id);
    free(stanza->to.local);
    free(stanza->to.domain);
    free(stanza->to.resource);
    free(stanza->type);
    free(stanza);
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
