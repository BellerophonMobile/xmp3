/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>

#include <netinet/in.h>

#include <expat.h>

#include "log.h"

// XML string constants
#define XMPP_NS_STREAM "http://etherx.jabber.org/streams"
#define XMPP_NS_SASL "urn:ietf:params:xml:ns:xmpp-sasl"
#define XMPP_NS_BIND "urn:ietf:params:xml:ns:xmpp-bind"
#define XMPP_NS_CLIENT "jabber:client"
#define XMPP_NS_SESSION "urn:ietf:params:xml:ns:xmpp-session"
#define XMPP_NS_DISCO_INFO "http://jabber.org/protocol/disco#info"
#define XMPP_NS_DISCO_ITEMS "http://jabber.org/protocol/disco#items"

const char *XMPP_STREAM;
const char *XMPP_AUTH;
const char *XMPP_BIND;
const char *XMPP_BIND_RESOURCE;

const char *XMPP_MESSAGE;
const char *XMPP_PRESENCE;
const char *XMPP_IQ;
const char *XMPP_IQ_SESSION;
const char *XMPP_IQ_QUERY_INFO;
const char *XMPP_IQ_QUERY_ITEMS;

const char *XMPP_ATTR_TO;
const char *XMPP_ATTR_FROM;
const char *XMPP_ATTR_ID;
const char *XMPP_ATTR_TYPE;
const char *XMPP_ATTR_TYPE_GET;
const char *XMPP_ATTR_TYPE_SET;
const char *XMPP_ATTR_TYPE_RESULT;
const char *XMPP_ATTR_TYPE_ERROR;

// Client data structures
struct jid {
    char *local;
    // domainpart is just "localhost"
    char *resource;
};

struct client_info {
    int fd;
    struct sockaddr_in caddr;
    XML_Parser parser;
    bool authenticated;
    struct jid jid;
};

struct common_attrs {
    char *id;
    char *to;
    char *from;
    char *type;
};

struct message_data {
    struct client_info *info;
    struct common_attrs attrs;
    XML_EndElementHandler end_handler;
};

struct presence_data {
    struct client_info *info;
    struct common_attrs attrs;
    XML_EndElementHandler end_handler;
};

struct iq_data {
    struct client_info *info;
    struct common_attrs attrs;
    XML_EndElementHandler end_handler;
};

// Callback function definitions
typedef void (*xep_message_handler)(struct message_data *data,
                                    const char *name, const char **attrs);
typedef void (*xep_presence_handler)(struct presence_data *data,
                                     const char *name, const char **attrs);
typedef void (*xep_iq_handler)(struct iq_data *data, const char *name,
                               const char **attrs);

/** Print out an XML start element and its attributes */
void xmpp_print_start_tag(const char *name, const char **attrs);

/** Print out an XML end element */
void xmpp_print_end_tag(const char *name);

/** Print out a string of XML data */
void xmpp_print_data(const char *s, int len);

/** Expat callback for when you do not expect a start tag. */
void xmpp_error_start(void *data, const char *name, const char **attrs);

/** Expat callback for when you do not expect an end tag. */
void xmpp_error_end(void *data, const char *name);

/** Expat callback for when you do not expect XML data. */
void xmpp_error_data(void *data, const char *s, int len);

void xmpp_handle_common_attrs(struct common_attrs *common_attrs,
                              const char **attrs);

void xmpp_del_common_attrs(struct common_attrs *common_attrs);
