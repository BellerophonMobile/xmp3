/**
 * xmp3 - XMPP Proxy
 * jid.{c,h} - Functions and data structure to manipulate a JID
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

/** Opaque pointer to a JID structure. */
struct jid;

/** Allocate and initialize a new blank JID. */
struct jid* jid_new();

/** Cleans up and frees an existing JID. */
void jid_del(struct jid *jid);

/**
 * Allocate and initialize a new JID from an existing JID string.
 *
 * This will copy parts of the input string.  You are responsible for cleaning
 * up the input string.
 *
 * @param jidstr A string of the form "localpart@domainpart/resourcepart"
 * @returns A new JID structure.
 */
struct jid* jid_new_from_str(const char *jidstr);

/**
 * Allocate and initialize a new JID from an existing JID structure.
 *
 * This copies a JID structure.
 *
 * @param jid The JID to copy.
 * @returns A new JID structure.
 */
struct jid* jid_new_from_jid(const struct *jid);

/**
 * Converts a JID to a newly allocated string.
 *
 * String is of the form "localpart@domainpart/resourcepart".
 *
 * @param jid JID structure to convert.
 * @returns A newly allocated string containing the JID, or NULL if the string
 *          is invalid.
 */
char* jid_to_str(const struct jid *jid);

/**
 * Compare two JIDs exactly.
 *
 * @returns 0 if exact match, -1 if a is before b in lexographically sorted
 *          order, -1 otherwise.
 */
int jid_cmp(const struct jid *a, const struct jid *b);

/**
 * Returns the localpart of a JID.
 *
 * The string returned is owned by the JID object, and should not be freed.
 *
 * @returns Pointer to the localpart JID string, or NULL if none is set.
 */
const char* jid_local(const struct jid *jid);

/**
 * Sets the localpart of a JID.
 *
 * The string will be copied into the JID.  The string you pass in is your
 * responsibility to clean up.
 */
void jid_set_local(struct jid *jid, const char *localpart);

/**
 * Returns the domainpart of a JID.
 *
 * The string returned is owned by the JID object, and should not be freed.
 *
 * @returns Pointer to the domainpart JID string, or NULL if none is set.
 */
const char* jid_domain(const struct jid *jid);

/**
 * Sets the domainpart of a JID.
 *
 * The string will be copied into the JID.  The string you pass in is your
 * responsibility to clean up.
 */
void jid_set_domain(struct jid *jid, const char *domainpart);

/**
 * Returns the resourcepart of a JID.
 *
 * The string returned is owned by the JID object, and should not be freed.
 *
 * @returns Pointer to the resourcepart JID string, or NULL if none is set.
 */
const char* jid_resource(const struct jid *jid);

/**
 * Sets the resourcepart of a JID.
 *
 * The string will be copied into the JID.  The string you pass in is your
 * responsibility to clean up.
 */
void jid_set_resource(struct jid *jid, const char *domainpart);
