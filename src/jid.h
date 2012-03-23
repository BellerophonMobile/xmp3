/*
 * Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file jid.h
 * Functions and data structure to manipulate a JID
 */

#pragma once

/** Opaque pointer to a JID structure. */
struct jid;

/** Allocate and initialize a new blank JID. */
struct jid* jid_new(void);

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
struct jid* jid_new_from_jid(const struct jid *jid);

/**
 * Allocate and initialize a new JID from an existing JID without the resource.
 *
 * This copies a JID structure.
 *
 * @param jid The JID to copy.
 * @returns A new JID structure.
 */
struct jid* jid_new_from_jid_bare(const struct jid *jid);

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

/** Gets the length of a JID string without converting it. */
int jid_to_str_len(const struct jid *jid);

/**
 * Compare two JIDs exactly.
 *
 * @returns 0 if exact match, -1 if a is before b in lexographically sorted
 *          order, -1 otherwise.
 */
int jid_cmp(const struct jid *a, const struct jid *b);

/**
 * Compare two JIDs with wildcard matches.
 *
 * @returns 0 if exact match, -1 if a is before b in lexographically sorted
 *          order, -1 otherwise.
 */
int jid_cmp_wildcards(const struct jid *a, const struct jid *b);

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
void jid_set_resource(struct jid *jid, const char *resourcepart);
