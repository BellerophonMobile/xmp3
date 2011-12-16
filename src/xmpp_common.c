/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <stdbool.h>
#include <stdio.h>

#include <expat.h>

#include "utstring.h"

#include "log.h"
#include "utils.h"

#include "client_socket.h"
#include "xmp3_xml.h"
#include "xmpp_stanza.h"
#include "xmpp_client.h"
#include "xmpp_server.h"

#include "xmpp_common.h"

const char *XMPP_STREAM = XMPP_NS_STREAM XMPP_NS_SEPARATOR "stream";
const char *XMPP_AUTH = XMPP_NS_SASL XMPP_NS_SEPARATOR "auth";
const char *XMPP_BIND = XMPP_NS_BIND XMPP_NS_SEPARATOR "bind";
const char *XMPP_BIND_RESOURCE = XMPP_NS_BIND XMPP_NS_SEPARATOR "resource";

const char *XMPP_MESSAGE = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "message";
const char *XMPP_MESSAGE_BODY = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "body";
const char *XMPP_PRESENCE = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "presence";
const char *XMPP_IQ = XMPP_NS_CLIENT XMPP_NS_SEPARATOR "iq";

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

void xmpp_error_start(void *data, const char *name, const char **attrs,
                      struct xmp3_xml *parser) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_err("Unexpected start tag %s", name);
    xmp3_xml_stop_parser(xmpp_client_parser(client), false);
}

void xmpp_error_end(void *data, const char *name, struct xmp3_xml *parser) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_err("Unexpected end tag %s", name);
    xmp3_xml_stop_parser(xmpp_client_parser(client), false);
}

void xmpp_error_data(void *data, const char *s, int len,
                     struct xmp3_xml *parser) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_err("Unexpected data");
    xmp3_xml_stop_parser(xmpp_client_parser(client), false);
}

void xmpp_ignore_start(void *data, const char *name, const char **attrs,
                       struct xmp3_xml *parser) {
    debug("Ignoring start tag");
}

void xmpp_ignore_data(void *data, const char *s, int len,
                      struct xmp3_xml *parser) {
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
    struct xmpp_client *client = xmpp_server_find_client(
            xmpp_stanza_server(stanza), xmpp_stanza_jid_from(stanza));

    if (client == NULL) {
        return;
    }

    UT_string *msg = NULL;
    utstring_new(msg);

    // Ignore the "to" and "from" attributes
    char *to = NULL;
    if (xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO) != NULL) {
        STRDUP_CHECK(to, xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO));
        xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TO, NULL);
    }

    char *from = NULL;
    if (xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM) != NULL) {
        STRDUP_CHECK(from, xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM));
        xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM, NULL);
    }

    // We need to change the type to "error"
    char *type = NULL;
    if (xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE) != NULL) {
        STRDUP_CHECK(type, xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE));
    }
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TYPE, XMPP_ATTR_TYPE_ERROR);

    char *start_tag = xmpp_stanza_create_tag(stanza);

    // Put everything back how it was
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TO, to);
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_FROM, from);
    xmpp_stanza_set_attr(stanza, XMPP_STANZA_ATTR_TYPE, type);
    if (to != NULL) {
        free(to);
    }
    if (from != NULL) {
        free(from);
    }
    if (type != NULL) {
        free(type);
    }

    utstring_printf(msg, start_tag);
    free(start_tag);

    utstring_printf(msg, error);

    utstring_printf(msg, "</%s>", xmpp_stanza_name(stanza));

    /* TODO: If this fails, we should probably close the connection or
     * something. */
    check(client_socket_sendall(xmpp_client_socket(client), utstring_body(msg),
                                utstring_len(msg)) > 0,
          "Error sending not supported error items");
    utstring_free(msg);
    return;

error:
    if (msg != NULL) {
        utstring_free(msg);
    }
    xmp3_xml_stop_parser(xmpp_stanza_parser(stanza), false);
}
