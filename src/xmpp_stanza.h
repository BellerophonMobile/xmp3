/**
 * xmp3 - XMPP Proxy
 * xmpp_stanza.{c,h} - Represents top-level XMPP stanzas received from clients.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

// Forward declarations
struct xmpp_stanza;

extern const char *XMPP_STANZA_NS_CLIENT;
extern const char *XMPP_STANZA_NS_STANZA;

extern const char *XMPP_STANZA_MESSAGE;
extern const char *XMPP_STANZA_PRESENCE;
extern const char *XMPP_STANZA_IQ;

extern const char *XMPP_STANZA_ATTR_TO;
extern const char *XMPP_STANZA_ATTR_FROM;
extern const char *XMPP_STANZA_ATTR_ID;
extern const char *XMPP_STANZA_ATTR_TYPE;

extern const char *XMPP_STANZA_TYPE_SET;
extern const char *XMPP_STANZA_TYPE_GET;
extern const char *XMPP_STANZA_TYPE_RESULT;
extern const char *XMPP_STANZA_TYPE_ERROR;

/**
 * Allocate and initialize a new XMPP stanza structure.
 *
 * @param ns_name The name of the stanza start tag.
 * @param attrs NULL terminated list attribute = value pairs.
 * @returns A new XMPP stanza structure.
 */
struct xmpp_stanza* xmpp_stanza_new(const char *ns_name, const char **attrs);

/** Cleans up and frees an XMPP stanza. */
void xmpp_stanza_del(struct xmpp_stanza *stanza, bool recursive);

/**
 * Returns the stanza (and its children) packed into a null-terminated string.
 *
 * @param stanza The stanza to convert.
 * @param len If not null, is filled in with the length of the string.
 */
char* xmpp_stanza_string(struct xmpp_stanza *stanza, size_t *len);

/**
 * Returns the namespace of this stanza.
 *
 * The string returned is owned by the stanza, and should not be freed.
 *
 * @param stanza The stanza structure.
 * @returns The the namespace of this stanza.
 */
const char* xmpp_stanza_namespace(const struct xmpp_stanza *stanza);

void xmpp_stanza_copy_namespace(struct xmpp_stanza *stanza, const char *ns);

/**
 * Returns the name of the tag of this stanza (message/presence/IQ)
 *
 * The string returned is owned by the stanza, and should not be freed.
 *
 * @param stanza The stanza structure.
 * @returns The name of this stanza.
 */
const char* xmpp_stanza_name(const struct xmpp_stanza *stanza);

void xmpp_stanza_copy_name(struct xmpp_stanza *stanza, const char *name);

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
 * The name/value are NOT copied.
 */
void xmpp_stanza_set_attr(struct xmpp_stanza *stanza, const char *name,
                          char *value);

/**
 * Sets the value of an attribute in a stanza.
 *
 * The name/value are copied.
 */
void xmpp_stanza_copy_attr(struct xmpp_stanza *stanza, const char *name,
                           const char *value);

/** Returns any data associated with this stanza. */
const char* xmpp_stanza_data(const struct xmpp_stanza *stanza);

/** Returns the length of any data associated with this stanza. */
unsigned int xmpp_stanza_data_length(const struct xmpp_stanza *stanza);

/** Copies data. */
void xmpp_stanza_append_data(struct xmpp_stanza *stanza, const char *buf,
                             int len);

/** Returns the number of children stanzas. */
int xmpp_stanza_children_length(const struct xmpp_stanza *stanza);

/** Returns a pointer to the first child stanza. */
struct xmpp_stanza* xmpp_stanza_children(struct xmpp_stanza *stanza);

/** Returns a pointer to the parent stanza. */
struct xmpp_stanza* xmpp_stanza_parent(struct xmpp_stanza *stanza);

/** Returns a pointer to the next sibling stanza. */
struct xmpp_stanza* xmpp_stanza_next(struct xmpp_stanza *stanza);

/** Returns a pointer to the previous sibling stanza. */
struct xmpp_stanza* xmpp_stanza_prev(struct xmpp_stanza *stanza);

/** Appends a child stanza. */
void xmpp_stanza_append_child(struct xmpp_stanza *stanza,
                              struct xmpp_stanza *child);
