/**
 * xmp3 - XMPP Proxy
 * xmpp_stanza.{c,h} - Represents top-level XMPP stanzas received from clients.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

extern const char *XMPP_STANZA_ATTR_TO;
extern const char *XMPP_STANZA_ATTR_FROM;
extern const char *XMPP_STANZA_ATTR_ID;
extern const char *XMPP_STANZA_ATTR_TYPE;

extern const char *XMPP_ATTR_TYPE_GET;
extern const char *XMPP_ATTR_TYPE_SET;
extern const char *XMPP_ATTR_TYPE_RESULT;
extern const char *XMPP_ATTR_TYPE_ERROR;


// Forward declarations
struct xmpp_stanza;
struct xmpp_server;
struct xmpp_client;
struct xmp3_xml;

/**
 * Allocate and initialize a new XMPP stanza structure.
 *
 * @param server The server this message is being processed by.
 * @param parser The XML parser that is parsing this stanza.
 * @param client An optional client that sent this stanza, may be NULL.
 * @param name The name of the stanza start tag.
 * @param attrs NULL terminated list attribute = value pairs.
 * @returns A new XMPP stanza structure.
 */
struct xmpp_stanza* xmpp_stanza_new(struct xmpp_server *server,
                                    struct xmp3_xml *parser,
                                    struct xmpp_client *client,
                                    const char *name, const char **attrs);

/** Cleans up and frees an XMPP stanza. */
void xmpp_stanza_del(struct xmpp_stanza *stanza);

/** Get the server who is processing this stanza. */
struct xmpp_server* xmpp_stanza_server(const struct xmpp_stanza *stanza);

/** Get the XML parser that is parsing this stanza. */
struct xmp3_xml* xmpp_stanza_parser(const struct xmpp_stanza *stanza);

/**
 * Returns the name of the tag of this stanza (message/presence/IQ)
 *
 * The string returned is owned by the stanza, and should not be freed.
 *
 * @param stanza The stanza structure.
 * @returns The name of this stanza.
 */
const char* xmpp_stanza_name(const struct xmpp_stanza *stanza);

/**
 * Returns the namespace of this stanza.
 *
 * The string returned is owned by the stanza, and should not be freed.
 *
 * @param stanza The stanza structure.
 * @returns The the namespace of this stanza.
 */
const char* xmpp_stanza_namespace(const struct xmpp_stanza *stanza);

/**
 * Returns the namespace and name combined, separated by #.
 *
 * This is mostly for convenience, so you can strcmp both in one go.
 */
const char* xmpp_stanza_ns_name(const struct xmpp_stanza *stanza);

/**
 * Returns the JID structure from the string JID attribute "from".
 *
 * The JID returned is owned by the stanza, and should not be freed.
 *
 * @param stanza The stanza structure.
 * @returns A new JID structure based on the "from" attribute of the stanza.
 *          If not present, NULL is returned.
 */
struct jid* xmpp_stanza_jid_from(const struct xmpp_stanza *stanza);

/**
 * Returns the JID structure from the string JID attribute "to".
 *
 * The JID returned is owned by the stanza, and should not be freed.
 *
 * @param stanza The stanza structure.
 * @returns A new JID structure based on the "from" attribute of the stanza.
 *          If not present, NULL is returned.
 */
struct jid* xmpp_stanza_jid_to(const struct xmpp_stanza *stanza);

/**
 * Returns the value of an attribute of this stanza.
 *
 * Use the constants defined in this file to get common attributes.
 *
 * @param stanza The stanza structure.
 * @param name The name of the attribute to look up
 * @returns The value of the attribute, or NULL if it doesn't exist.
 */
const char* xmpp_stanza_attr(const struct xmpp_stanza *stanza,
                             const char *name);

/**
 * Sets the value of an attribute in a stanza.
 *
 * The name/value are copied if necessary.
 */
void xmpp_stanza_set_attr(struct xmpp_stanza *stanza, const char *name,
                          const char *value);

/**
 * Takes a stanza struct and recreates the string tag.
 *
 * @param stanza Stanza structure to create a tag for.
 * @return A newly allocated string representing the start stanza tag, with all
 *         attributes and their values.
 */
char* xmpp_stanza_create_tag(const struct xmpp_stanza *stanza);
