/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xmpp_core.h"

#include <expat.h>

#include "utstring.h"

#include "utils.h"
#include "xmpp.h"

/** Temporary structure for reading a message stanza. */
struct message_tmp {
    struct xmpp_client *to_client;
    struct xmpp_stanza *from_stanza;
};

// Forward declarations
static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                      const char *name, const char **attrs);
static void del_stanza(struct xmpp_stanza *stanza);

// Message stanzas
static void message_client_start(void *data, const char *name,
                                 const char **attrs);
static void message_client_end(void *data, const char *name);
static void message_client_data(void *data, const char *s, int len);

static void iq_start(void *data, const char *name, const char **attrs);

void xmpp_core_stanza_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    struct xmpp_stanza *stanza = new_stanza(client, name, attrs);
    XML_SetUserData(client->parser, stanza);

    /* We expect any stanza callbacks to change the XML callbacks if
     * neccessary, else the whole stanza is ignored. */
    XML_SetElementHandler(client->parser, xmpp_ignore_start,
                          xmpp_core_stanza_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);

    if (!xmpp_route_stanza(stanza)) {
        log_warn("Unhandled stanza");
    }
}

void xmpp_core_stanza_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;

    if (strcmp(name, stanza->ns_name) != 0) {
        return;
    }

    log_info("Stanza end");

    // Clean up our stanza structure.
    XML_SetUserData(client->parser, client);
    del_stanza(stanza);

    // We expect to see the start of a new stanza next.
    XML_SetElementHandler(client->parser, xmpp_core_stanza_start,
            xmpp_core_stream_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);
}

void xmpp_core_stream_end(void *data, const char *name) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");
    client->connected = false;
    return;

error:
    XML_StopParser(client->parser, false);
}

bool xmpp_core_stanza_handler(struct xmpp_stanza *stanza, void *data) {
    if (strcmp(stanza->ns_name, XMPP_MESSAGE) == 0) {
        log_warn("Message addressed to server?");
    } else if (strcmp(stanza->ns_name, XMPP_PRESENCE) == 0) {
        // TODO: Handle presence stanzas.
        log_info("Presence stanza");
        //handle_presence(stanza, attrs);
        return false;
    } else if (strcmp(stanza->ns_name, XMPP_IQ) == 0) {
        log_info("IQ stanza start");
        XML_SetStartElementHandler(stanza->from_client->parser, iq_start);
        return true;
    } else {
        log_warn("Unknown stanza");
    }
    return false;
}

bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data) {
    struct xmpp_client *to_client = (struct xmpp_client*)data;
    struct xmpp_client *from_client = from_stanza->from_client;

    if (strcmp(from_stanza->ns_name, XMPP_MESSAGE) != 0) {
        return false;
    }

    // Set the XML handlers to expect the rest of the message stanza.
    XML_SetElementHandler(from_client->parser, message_client_start,
                          message_client_end);
    XML_SetCharacterDataHandler(from_client->parser, message_client_data);

    /* We need to add a "from" field to the outgoing stanza regardless of
     * whether or not the client gave us one. */
    char *fromstr = from_stanza->from;
    from_stanza->from = jid_to_str(&from_client->jid);
    char* msg = create_start_tag(from_stanza);
    free(from_stanza->from);
    from_stanza->from = fromstr;

    if (sendall(to_client->socket, msg, strlen(msg)) <= 0) {
        log_err("Error sending message start tag to destination.");
    }

    struct message_tmp *tmp = calloc(1, sizeof(*tmp));
    tmp->to_client = to_client;
    tmp->from_stanza = from_stanza;
    XML_SetUserData(from_client->parser, tmp);

    free(msg);
    return true;
}

/** Allocates an initializes a new XMPP stanza struct. */
static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                      const char *name,
                                      const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);

    utarray_new(stanza->other_attrs, &ut_str_icd);

    ALLOC_COPY_STRING(stanza->ns_name, name);
    char *ns_delim = strrchr(name, *XMPP_NS_SEPARATOR);
    if (ns_delim == NULL) {
        ALLOC_COPY_STRING(stanza->name, name);
    } else {
        int ns_len = ns_delim - name;
        stanza->namespace = calloc(ns_len + 1, sizeof(*stanza->namespace));
        check_mem(stanza->namespace);
        strncpy(stanza->namespace, name, ns_len);

        int name_len = strlen(name) - ns_len - 1;
        stanza->name = calloc(name_len + 1, sizeof(*stanza->name));
        check_mem(stanza->name);
        strncpy(stanza->name, name + ns_len + 1, name_len);
    }

    /* RFC6120 Section 8.1.2.1 states that the server is to basically ignore
     * any "from" attribute in the stanza, and append the JID of the client the
     * server got the stanza from. */
    stanza->from_client = client;

    for (int i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_ID) == 0) {
            ALLOC_COPY_STRING(stanza->id, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TO) == 0) {
            ALLOC_COPY_STRING(stanza->to, attrs[i + 1]);
            str_to_jid(attrs[i + 1], &stanza->to_jid);
        } else if (strcmp(attrs[i], XMPP_ATTR_FROM) == 0) {
            ALLOC_COPY_STRING(stanza->from, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            ALLOC_COPY_STRING(stanza->type, attrs[i + 1]);
        } else {
            ALLOC_PUSH_BACK(stanza->other_attrs, attrs[i]);
            ALLOC_PUSH_BACK(stanza->other_attrs, attrs[i + 1]);
        }
    }

    /* RFC6120 Section 10.3 specifies where a stanza should be delivered if it
     * has no "to" address. */
    if (stanza->to == NULL) {
        if (strcmp(stanza->ns_name, XMPP_MESSAGE) == 0) {
            // Addressed to bare JID of the sending entity
            ALLOC_COPY_STRING(stanza->to_jid.local, client->jid.local);
            ALLOC_COPY_STRING(stanza->to_jid.domain, client->jid.domain);
        } else {
            // Otherwise, its addressed to the server itself.
            str_to_jid(SERVER_DOMAIN, &stanza->to_jid);
        }
        stanza->to = jid_to_str(&stanza->to_jid);
    }

    return stanza;
}

/** Cleans up and frees an XMPP stanza struct. */
static void del_stanza(struct xmpp_stanza *stanza) {
    free(stanza->ns_name);
    free(stanza->name);
    free(stanza->namespace);
    free(stanza->id);
    free(stanza->to);
    free(stanza->from);
    free(stanza->type);
    free(stanza->to_jid.local);
    free(stanza->to_jid.domain);
    free(stanza->to_jid.resource);

    char **p = NULL;
    while ((p = (char**)utarray_next(stanza->other_attrs, p)) != NULL) {
        free(*p);
    }
    utarray_free(stanza->other_attrs);
    free(stanza);
}

/** Handles a starting tag inside of a <message> stanza. */
static void message_client_start(void *data, const char *name,
                                 const char **attrs) {
    struct message_tmp *tmp = (struct message_tmp*)data;

    log_info("Client message start");

    if (sendxml(tmp->from_stanza->from_client->parser,
                tmp->to_client->socket) <= 0) {
        log_err("Error sending message to destination.");
    }
}

/** Handles an end tag inside of a <message> stanza. */
static void message_client_end(void *data, const char *name) {
    struct message_tmp *tmp = (struct message_tmp*)data;
    struct xmpp_client *from_client = tmp->from_stanza->from_client;

    log_info("Client message end");

    /* For self-closing tags "<foo/>" we get an end event without any data to
     * send.  We already sent the end tag with the start tag. */
    if (XML_GetCurrentByteCount(from_client->parser) > 0) {
        if (sendxml(from_client->parser, tmp->to_client->socket) <= 0) {
            log_err("Error sending message to destination.");
        }
    }

    // If we received a </message> tag, we are done parsing this stanza.
    if (strcmp(name, XMPP_MESSAGE) == 0) {
        XML_SetUserData(tmp->from_stanza->from_client->parser,
                        tmp->from_stanza);
        xmpp_core_stanza_end(tmp->from_stanza, name);
        free(tmp);
    }
}

/** Handles character data inside of a <message> stanza. */
static void message_client_data(void *data, const char *s, int len) {
    struct message_tmp *tmp = (struct message_tmp*)data;
    log_info("Client message data");

    if (sendxml(tmp->from_stanza->from_client->parser,
                tmp->to_client->socket) <= 0) {
        log_err("Error sending message to destination.");
    }
}

/** Handles the routing of an IQ stanza. */
static void iq_start(void *data, const char *name, const char **attrs) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from_client;

    /* Routing for IQ stanzas is based on the namespace (and tag name) of the
     * first child of the top IQ tag. */
    if (!xmpp_route_iq(name, stanza)) {
        /* If we can't answer the IQ, then let the client know.  Make sure to
         * ignore anything inside this stanza. */
        xmpp_send_service_unavailable(stanza);
        XML_SetElementHandler(client->parser, xmpp_ignore_start,
                              xmpp_core_stanza_end);
        XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);
    }
}
