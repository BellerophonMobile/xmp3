/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xmpp_common.h"

#include <stdio.h>

#include "utstring.h"

#include "utils.h"

const char *SERVER_DOMAIN = "localhost";
const struct jid SERVER_JID = {NULL, "localhost", NULL};

const char *XMPP_STREAM = XMPP_NS_STREAM XMPP_NS_SEPARATOR "stream";
const char *XMPP_AUTH = XMPP_NS_SASL XMPP_NS_SEPARATOR "auth";
const char *XMPP_BIND = XMPP_NS_BIND XMPP_NS_SEPARATOR "bind";
const char *XMPP_BIND_RESOURCE = XMPP_NS_BIND XMPP_NS_SEPARATOR "resource";

const char *XMPP_MESSAGE = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "message";
const char *XMPP_MESSAGE_BODY = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "body";
const char *XMPP_PRESENCE = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "presence";
const char *XMPP_IQ = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "iq";

const char *XMPP_ATTR_TO = "to";
const char *XMPP_ATTR_FROM = "from";
const char *XMPP_ATTR_ID = "id";
const char *XMPP_ATTR_TYPE = "type";
const char *XMPP_ATTR_TYPE_GET = "get";
const char *XMPP_ATTR_TYPE_SET = "set";
const char *XMPP_ATTR_TYPE_RESULT = "result";
const char *XMPP_ATTR_TYPE_ERROR = "error";

static const char *MSG_NOT_IMPLEMENTED =
    "<error type='cancel'>"
        "<feature-not-implemented "
            "xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
    "</error>";

static const char *MSG_SERVICE_UNAVAILABLE =
    "<error type='cancel'>"
        "<service-unavailable xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
    "</error>";

static void send_error(struct xmpp_stanza *stanza, const char *error);

void xmpp_print_start_tag(const char *name, const char **attrs) {
    printf("\t<%s", name);

    for (int i = 0; attrs[i] != NULL; i += 2) {
        printf(" %s=\"%s\"", attrs[i], attrs[i + 1]);
    }
    printf(">\n");
}

void xmpp_print_end_tag(const char *name) {
    printf("\t</%s>\n", name);
}

void xmpp_print_data(const char *s, int len) {
    /* The %1$d means take the first argument (len) and format it as a decimal.
     * The %2$.*1$s:
     *   %2$ - Take the second argument (s)
     *   .*1$ - Use the first argument for the length (len)
     *   s    - Format it as a string (using len as length)
     */
    printf("\t%1$d bytes: '%2$.*1$s'\n", len, s);
}

void xmpp_error_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_err("Unexpected start tag %s", name);
    XML_StopParser(client->parser, false);
}

void xmpp_error_end(void *data, const char *name) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_err("Unexpected end tag %s", name);
    XML_StopParser(client->parser, false);
}

void xmpp_error_data(void *data, const char *s, int len) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_err("Unexpected data");
    XML_StopParser(client->parser, false);
}

void xmpp_ignore_start(void *data, const char *name, const char **attrs) {
    debug("Ignoring start tag");
}

void xmpp_ignore_data(void *data, const char *s, int len) {
    debug("Ignoring data");
}

void xmpp_send_feature_not_implemented(struct xmpp_stanza *stanza) {
    log_warn("Sending feature not implemented");
    send_error(stanza, MSG_NOT_IMPLEMENTED);
}

void xmpp_send_service_unavailable(struct xmpp_stanza *stanza) {
    log_warn("Sending service unavailable");
    send_error(stanza, MSG_SERVICE_UNAVAILABLE);
}

/** Generic code to send errors. */
static void send_error(struct xmpp_stanza *stanza, const char *error) {
    struct xmpp_client *client = stanza->from_client;

    UT_string *msg = NULL;
    utstring_new(msg);

    // Ignore the "to" and "from" attributes
    char *to = stanza->to;
    stanza->to = NULL;
    char *from = stanza->from;
    stanza->from = NULL;

    // We need to change the type to "error"
    char *type = stanza->type;
    stanza->type = calloc(strlen(XMPP_ATTR_TYPE_ERROR) + 1,
                          sizeof(*stanza->type));
    check_mem(stanza->type);
    strcpy(stanza->type, XMPP_ATTR_TYPE_ERROR);

    char *start_tag = create_start_tag(stanza);

    // Put everything back how it was
    stanza->to = to;
    stanza->from = from;
    free(stanza->type);
    stanza->type = type;

    utstring_printf(msg, start_tag);
    free(start_tag);

    utstring_printf(msg, error);

    utstring_printf(msg, "</%s>", stanza->name);

    /* TODO: If this fails, we should probably close the connection or
     * something. */
    check(sendall(client->socket, utstring_body(msg), utstring_len(msg)) > 0,
          "Error sending not supported error items");

    // Explicit fallthrough
error:
    if (msg != NULL) {
        utstring_free(msg);
    }
}
