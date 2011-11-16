/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>
#include <netinet/in.h>

#include <openssl/ssl.h>

#include <expat.h>

#include "utarray.h"

#include "log.h"

// XML string constants
#define XMPP_NS_SEPARATOR "#"

#define XMPP_NS_STREAM "http://etherx.jabber.org/streams"
#define XMPP_NS_SASL "urn:ietf:params:xml:ns:xmpp-sasl"
#define XMPP_NS_BIND "urn:ietf:params:xml:ns:xmpp-bind"
#define XMPP_NS_CLIENT "jabber:client"
#define XMPP_NS_SESSION "urn:ietf:params:xml:ns:xmpp-session"
#define XMPP_NS_DISCO_ITEMS "http://jabber.org/protocol/disco#items"
#define XMPP_NS_DISCO_INFO "http://jabber.org/protocol/disco#info"
#define XMPP_NS_ROSTER "jabber:iq:roster"

const char *SERVER_DOMAIN;

const char *XMPP_STREAM;
const char *XMPP_AUTH;
const char *XMPP_BIND;
const char *XMPP_BIND_RESOURCE;

const char *XMPP_MESSAGE;
const char *XMPP_MESSAGE_BODY;
const char *XMPP_PRESENCE;
const char *XMPP_IQ;

const char *XMPP_ATTR_TO;
const char *XMPP_ATTR_FROM;
const char *XMPP_ATTR_ID;
const char *XMPP_ATTR_TYPE;
const char *XMPP_ATTR_TYPE_GET;
const char *XMPP_ATTR_TYPE_SET;
const char *XMPP_ATTR_TYPE_RESULT;
const char *XMPP_ATTR_TYPE_ERROR;

/** Represents a client ID (local@domain/resource). */
struct jid {
    char *local;
    char *domain;
    char *resource;
};

/** Data on a connected client. */
struct xmpp_client {
    /** The connected client_socket structure. */
    struct client_socket* socket;

    /** The address of the client. */
    struct sockaddr_in caddr;

    /** An Expat parser instance for this client. */
    XML_Parser parser;

    /** A pointer to the server this client is connected to. */
    struct xmpp_server *server;

    /** Whether the client has successfully authenticated or not. */
    bool authenticated;

    /** Whether this client is still connected or not. */
    bool connected;

    /** The JID of this client. */
    struct jid jid;

    /** @{ Clients are stored in a doubly-linked list. */
    struct xmpp_client *prev;
    struct xmpp_client *next;
    /** @} */
};

/** Represents a top-level XMPP stanza. */
struct xmpp_stanza {
    /** The name + namespace string. */
    char *ns_name;

    /** The name of this tag. */
    char *name;

    /** The namespace of this tag. */
    char *namespace;

    /** The "id" attribute of this tag (may be NULL). */
    char *id;

    /** The "to" attribute of this tag (may be NULL). */
    char *to;

    /** The "to" attribute converted to a JID structure. */
    struct jid to_jid;

    /** The "from" attribute of this tag (may be NULL). */
    char *from;

    /** The client structure that this stanza was received from. */
    struct xmpp_client *from_client;

    /** The "type" attribute of this tag (may be NULL). */
    char *type;

    /** Any other attributes attached to this tag. */
    UT_array *other_attrs;
};

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

/** Expat callback to ignore start tags. */
void xmpp_ignore_start(void *data, const char *name, const char **attrs);

/** Expat callback to ignore data. */
void xmpp_ignore_data(void *data, const char *s, int len);

/** Sends a "<feature-not-implemented>" stanza to a client. */
void xmpp_send_feature_not_implemented(struct xmpp_stanza *stanza);

/** Sends a "<service-unavailable>" stanza to a client. */
void xmpp_send_service_unavailable(struct xmpp_stanza *stanza);
