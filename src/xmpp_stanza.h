/**
 * xmp3 - XMPP Proxy
 * xmpp_stanza.{c,h} - Represents top-level XMPP stanzas received from clients.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include "xmpp_client.h"
#include "jid.h"

extern const char *XMPP_STANZA_ATTR_TO;
extern const char *XMPP_STANZA_ATTR_FROM;
extern const char *XMPP_STANZA_ATTR_ID;
extern const char *XMPP_STANZA_ATTR_TYPE;

/** Opaque pointer to an XMPP stanza structure. */
struct xmpp_stanza;

/**
 * Allocate and initialize a new XMPP stanza structure.
 *
 * @param client The client who sent this stanza.  The stanza DOES NOT take
 *               ownership of the client.
 * @param name The name of the stanza start tag.
 * @param attrs NULL terminated list attribute = value pairs.
 * @returns A new XMPP stanza structure.
 */
struct xmpp_stanza* xmpp_stanza_new(struct xmpp_client *client,
                                    const char *name, const char **attrs);

/** Cleans up and frees an XMPP stanza. */
void xmpp_stanza_del(struct xmpp_stanza *stanza);

/** Get the client who sent this stanza. */
struct xmpp_client* xmpp_stanza_client(const struct xmpp_stanza *stanza);

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
const char* xmpp_stanza_name_namespace(const struct xmpp_stanza *stanza);

/**
 * Returns a new JID structure from the string JID attribute "from".
 *
 * The JID returned is owned by the caller, and should be freed when done.
 *
 * @param stanza The stanza structure.
 * @returns A new JID structure based on the "from" attribute of the stanza.
 *          If not present, NULL is returned.
 */
struct jid* xmpp_stanza_jid_from(const struct xmpp_stanza *stanza);

/**
 * Returns a new JID structure from the string JID attribute "to".
 *
 * The JID returned is owned by the caller, and should be freed when done.
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
