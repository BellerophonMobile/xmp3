/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_common.h"

#include <stdio.h>

#define NS_STREAM "http://etherx.jabber.org/streams"
#define NS_SASL "urn:ietf:params:xml:ns:xmpp-sasl"
#define NS_BIND "urn:ietf:params:xml:ns:xmpp-bind"
#define NS_CLIENT "jabber:client"
#define NS_SESSION "urn:ietf:params:xml:ns:xmpp-session"
#define NS_DISCO_INFO "http://jabber.org/protocol/disco#info"
#define NS_DISCO_ITEMS "http://jabber.org/protocol/disco#items"

const char *XMPP_NS_STREAM = NS_STREAM;
const char *XMPP_NS_SASL = NS_SASL;
const char *XMPP_NS_BIND = NS_BIND;
const char *XMPP_NS_CLIENT = NS_CLIENT;
const char *XMPP_NS_SESSION = NS_SESSION;
const char *XMPP_NS_DISCO_INFO = NS_DISCO_INFO;
const char *XMPP_NS_DISCO_ITEMS = NS_DISCO_ITEMS;

const char *XMPP_STREAM = NS_STREAM " stream";
const char *XMPP_AUTH = NS_SASL " auth";
const char *XMPP_BIND = NS_BIND " bind";
const char *XMPP_BIND_RESOURCE = NS_BIND " resource";

const char *XMPP_MESSAGE = NS_CLIENT " message";
const char *XMPP_PRESENCE = NS_CLIENT " presence";
const char *XMPP_IQ = NS_CLIENT " iq";
const char *XMPP_IQ_SESSION = NS_SESSION " session";
const char *XMPP_IQ_QUERY_INFO = NS_DISCO_INFO " query";
const char *XMPP_IQ_QUERY_ITEMS = NS_DISCO_ITEMS " query";

const char *XMPP_ATTR_TO = "to";
const char *XMPP_ATTR_FROM = "from";
const char *XMPP_ATTR_ID = "id";
const char *XMPP_ATTR_TYPE = "type";
const char *XMPP_ATTR_TYPE_GET = "get";
const char *XMPP_ATTR_TYPE_SET = "set";
const char *XMPP_ATTR_TYPE_RESULT = "result";
const char *XMPP_ATTR_TYPE_ERROR = "error";

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
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected start tag %s", name);
    xmpp_print_start_tag(name, attrs);
    XML_StopParser(info->parser, false);
}

void xmpp_error_end(void *data, const char *name) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected end tag %s", name);
    xmpp_print_end_tag(name);
    XML_StopParser(info->parser, false);
}

void xmpp_error_data(void *data, const char *s, int len) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected data");
    xmpp_print_data(s, len);
    XML_StopParser(info->parser, false);
}
