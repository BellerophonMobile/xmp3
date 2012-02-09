/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

// XML string constants
#define XMPP_NS_SEPARATOR "#"

#define XMPP_NS_STREAM "http://etherx.jabber.org/streams"
#define XMPP_NS_SASL "urn:ietf:params:xml:ns:xmpp-sasl"
#define XMPP_NS_ROSTER "jabber:iq:roster"

extern const char *SERVER_DOMAIN;

extern const char *XMPP_STREAM;
extern const char *XMPP_AUTH;
extern const char *XMPP_BIND;
extern const char *XMPP_BIND_RESOURCE;

extern const char *XMPP_MESSAGE;
extern const char *XMPP_MESSAGE_BODY;
extern const char *XMPP_PRESENCE;
extern const char *XMPP_IQ;

// Forward declarations
struct xmpp_stanza;
struct xmp3_xml;

/** Print out an XML start element and its attributes */
void xmpp_print_start_tag(const char *name, const char **attrs);

/** Print out an XML end element */
void xmpp_print_end_tag(const char *name);

/** Print out a string of XML data */
void xmpp_print_data(const char *s, int len);

/** Expat callback for when you do not expect a start tag. */
void xmpp_error_start(void *data, const char *name, const char **attrs,
                      struct xmp3_xml *parser);

/** Expat callback for when you do not expect an end tag. */
void xmpp_error_end(void *data, const char *name, struct xmp3_xml *parser);

/** Expat callback for when you do not expect XML data. */
void xmpp_error_data(void *data, const char *s, int len,
                     struct xmp3_xml *parser);

/** Expat callback to ignore start tags. */
void xmpp_ignore_start(void *data, const char *name, const char **attrs,
                       struct xmp3_xml *parser);

/** Expat callback to ignore data. */
void xmpp_ignore_data(void *data, const char *s, int len,
                      struct xmp3_xml *parser);

/** Sends a "<feature-not-implemented>" stanza to a client. */
void xmpp_send_feature_not_implemented(struct xmpp_stanza *stanza);

/** Sends a "<service-unavailable>" stanza to a client. */
void xmpp_send_service_unavailable(struct xmpp_stanza *stanza);
