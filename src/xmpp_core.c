/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_core.h"

#include <expat.h>

#include "utstring.h"

#include "utils.h"
#include "xmpp.h"

/* This should probably go in the utils module, if I use it outside of here,
 * I will. */
#define ALLOC_COPY_STRING(a, b) do { \
    a = calloc(strlen(b) + 1, sizeof(char)); \
    check_mem(a); \
    strcpy(a, b); \
} while (0)

#define ALLOC_PUSH_BACK(a, b) do { \
    char *tmp = calloc(strlen(b), sizeof(*tmp)); \
    check_mem(tmp); \
    utarray_push_back(a, tmp); \
} while (0)

static const char *MSG_MESSAGE_START = "<message";

// Forward declarations
static void new_stanza_jid(struct jid *jid, const char *strjid);
static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                      const char *name, const char **attrs);
static void del_stanza(struct xmpp_stanza *stanza);

// Message stanzas
static void message_client_start(void *data, const char *name,
                                 const char **attrs);
static void message_client_end(void *data, const char *name);
static void message_client_data(void *data, const char *s, int len);

void xmpp_core_stanza_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    struct xmpp_stanza *stanza = new_stanza(client, name, attrs);
    XML_SetUserData(client->parser, stanza);
    XML_SetElementHandler(client->parser, xmpp_ignore_start,
                          xmpp_core_stanza_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        log_info("Message stanza start");
        xmpp_route_message(stanza);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        log_info("Presence stanza start");
        //handle_presence(stanza, attrs);
    } else if (strcmp(name, XMPP_IQ) == 0) {
        log_info("IQ stanza start");
        //handle_iq(stanza, attrs);
    } else {
        log_warn("Unknown stanza");
    }
}

void xmpp_core_stanza_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;

    if (strcmp(name, stanza->name) == 0) {
        log_info("Stanza end");
        XML_SetUserData(client->parser, client);
        del_stanza(stanza);

        XML_SetElementHandler(client->parser, xmpp_core_stanza_start,
                xmpp_core_stream_end);
        XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);
    } else {
        log_info("Ignoring end tag");
    }
}

void xmpp_core_stream_end(void *data, const char *name) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");
    client->connected = false;
    return;

error:
    XML_StopParser(client->parser, false);
}

bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data) {
    struct xmpp_client *to_client = (struct xmpp_client*)data;
    struct xmpp_client *from_client = from_stanza->from_client;
    XML_SetElementHandler(from_client->parser, message_client_start,
                          message_client_end);
    XML_SetCharacterDataHandler(from_client->parser, message_client_data);

    /* We need to add a "from" field to the outgoing stanza regardless of
     * whether or not the client gave us one. */
    UT_string *msg;
    utstring_new(msg);

    utstring_printf(msg, MSG_MESSAGE_START);

    UT_string *from = jid_to_str(&from_stanza->from_client->jid);
    utstring_printf(msg, " %s='%s'", XMPP_ATTR_FROM, utstring_body(from));
    utstring_free(from);

    UT_string *to = jid_to_str(&from_stanza->to_jid);
    utstring_printf(msg, " %s='%s'", XMPP_ATTR_TO, utstring_body(to));
    utstring_free(to);

    for (int i = 0; i < utarray_len(from_stanza->other_attrs); i += 2) {
        utstring_printf(msg, " %s='%s'",
                utarray_eltptr(from_stanza->other_attrs, i),
                utarray_eltptr(from_stanza->other_attrs, i + 1));
    }

    if (sendall(to_client->fd, utstring_body(msg), utstring_len(msg)) <= 0) {
        log_err("Error sending message start tag to destination.");
    }

    utstring_free(msg);
    return true;
}

static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                           const char *name,
                                           const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);

    utarray_new(stanza->other_attrs, &ut_str_icd);

    ALLOC_COPY_STRING(stanza->name, name);

    /* RFC6120 Section 8.1.2.1 states that the server is to basically ignore
     * any "from" attribute in the stanza, and append the JID of the client the
     * server got the stanza from. */
    stanza->from_client = client;

    for (int i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_ID) == 0) {
            ALLOC_COPY_STRING(stanza->id, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TO) == 0) {
            new_stanza_jid(&stanza->to_jid, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            ALLOC_COPY_STRING(stanza->type, attrs[i + 1]);
        } else {
            ALLOC_PUSH_BACK(stanza->other_attrs, attrs[i]);
            ALLOC_PUSH_BACK(stanza->other_attrs, attrs[i + 1]);
        }
    }
    return stanza;
}

static void del_stanza(struct xmpp_stanza *stanza) {
    free(stanza->name);
    free(stanza->id);
    free(stanza->to_jid.local);
    free(stanza->to_jid.domain);
    free(stanza->to_jid.resource);
    free(stanza->type);

    char **p = NULL;
    while ((p = (char**)utarray_next(stanza->other_attrs, p)) != NULL) {
        free(*p);
    }
    utarray_free(stanza->other_attrs);
    free(stanza);
}

static void message_client_start(void *data, const char *name,
                                 const char **attrs) {
    //struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Client message start");
    //route_message(stanza);
}

static void message_client_end(void *data, const char *name) {
    //struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    //struct xmpp_client *client = stanza->from;

    log_info("Client message end");
    //route_message(stanza);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        xmpp_core_stanza_end(data, name);
    }
    return;
}

static void message_client_data(void *data, const char *s, int len) {
    //struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    log_info("Client message data");
    xmpp_print_data(s, len);
    //route_message(stanza);
}
