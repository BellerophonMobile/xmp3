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
 * @file jid.c
 * Defines structures and functions for XMP3 extension modules.
 * Functions and data structure to manipulate a JID
 */

#include <ctype.h>

#include <utstring.h>

#include "log.h"
#include "utils.h"

#include "jid.h"

/* From RFC 6122 Section 2.1. */
const int JID_PART_MAX_LEN = 1023;

/* (local + domain + resource) + '@' + '/'. */
const int JID_MAX_LEN = 3071;

/** Represents a JID (local@domain/resource). */
struct jid {
    char *local;
    char *domain;
    char *resource;
};

struct jid* jid_new(void) {
    struct jid *jid = calloc(1, sizeof(*jid));
    return jid;
}

void jid_del(struct jid *jid) {
    free(jid->local);
    free(jid->domain);
    free(jid->resource);
    free(jid);
}

struct jid* jid_new_from_str(const char *jidstr) {
    struct jid *jid = jid_new();

    const char *at_delim = strchr(jidstr, '@');
    const char *slash_delim = strchr(jidstr, '/');

    if (at_delim != NULL) {
        int len = at_delim - jidstr;
        if (len > JID_PART_MAX_LEN) {
            len = JID_PART_MAX_LEN;
        }
        check(len > 0, "JID local part cannot be empty.");
        STRNDUP_CHECK(jid->local, jidstr, len);
        jidstr = at_delim + 1;
    }

    if (slash_delim != NULL) {
        int len = slash_delim - jidstr;
        if (len > JID_PART_MAX_LEN) {
            len = JID_PART_MAX_LEN;
        }
        check(len > 0, "JID domain part cannot be empty.");
        STRNDUP_CHECK(jid->domain, jidstr, len);

        check(slash_delim[1] != '\0', "JID resource part cannot be empty.");
        STRNDUP_CHECK(jid->resource, slash_delim + 1, JID_PART_MAX_LEN);
    } else {
        check(jidstr[0] != '\0', "JID domain part cannot be empty.");
        STRNDUP_CHECK(jid->domain, jidstr, JID_PART_MAX_LEN);
    }
    return jid;

error:
    jid_del(jid);
    return NULL;
}

struct jid* jid_new_from_jid(const struct jid *jid) {
    struct jid *newjid = jid_new();
    jid_set_local(newjid, jid_local(jid));
    jid_set_domain(newjid, jid_domain(jid));
    jid_set_resource(newjid, jid_resource(jid));

    return newjid;
}

struct jid* jid_new_from_jid_bare(const struct jid *jid) {
    struct jid *newjid = jid_new();
    jid_set_local(newjid, jid_local(jid));
    jid_set_domain(newjid, jid_domain(jid));

    return newjid;
}

char* jid_to_str(const struct jid *jid) {
    /* Domain part is required */
    if (jid->domain == NULL) {
        return NULL;
    }

    UT_string strjid;
    utstring_init(&strjid);

    if (jid->local == NULL) {
        utstring_printf(&strjid, jid->domain);
    } else {
        utstring_printf(&strjid, "%s@%s", jid->local, jid->domain);
    }

    if (jid->resource != NULL) {
        utstring_printf(&strjid, "/%s", jid->resource);
    }

    /* Explicitly using utstring_init instead of utstring_new cleans up the
     * UT_string structure when the function exits.  Since we never call
     * utstring_done or utstring_free, the actual string being created inside
     * the structure is never freed, so this does not leak memory. */
    return utstring_body(&strjid);
}

int jid_to_str_len(const struct jid *jid) {
    int len = strlen(jid->domain);
    if (jid->local) {
        /* +1 for '@' */
        len += strlen(jid->local) + 1;
    }
    if (jid->resource) {
        /* +1 for '/' */
        len += strlen(jid->resource) + 1;
    }
    return len;
}

int jid_cmp(const struct jid *a, const struct jid *b) {
    /* If one has a local part, and the other doesn't then no match. */
    if ((a->local == NULL) != (b->local == NULL)) {
        return a->local == NULL ? -1 : 1;
    }

    if (a->local != NULL && b->local != NULL) {
        int rv = strncmp(a->local, b->local, JID_PART_MAX_LEN);
        if (rv != 0) {
            return rv;
        }
    }

    /* Domains should never be NULL, but just to be sure. */
    if ((a->domain == NULL) != (b->domain == NULL)) {
        return a->domain == NULL ? -1 : 1;
    }

    if (a->domain != NULL && b->domain != NULL) {
        int rv = strncmp(a->domain, b->domain, JID_PART_MAX_LEN);
        if (rv != 0) {
            return rv;
        }
    }

    if ((a->resource == NULL) != (b->resource == NULL)) {
        return a->resource == NULL ? -1 : 1;
    }

    if (a->resource != NULL && b->resource != NULL) {
        return strncmp(a->resource, b->resource, JID_PART_MAX_LEN);
    }

    return 0;
}

int jid_cmp_wildcards(const struct jid *a, const struct jid *b) {
    if (a->local == NULL) {
        if (b->local != NULL && strncmp(b->local, "*", JID_PART_MAX_LEN)
                != 0) {
            return 1;
        }
    }
    if (b->local == NULL) {
        if (a->local != NULL && strncmp(a->local, "*", JID_PART_MAX_LEN)
                != 0) {
            return 1;
        }
    }
    if (a->local != NULL && b->local != NULL
            && strncmp(a->local, "*", JID_PART_MAX_LEN) != 0
            && strncmp(b->local, "*", JID_PART_MAX_LEN) != 0) {
        int rv = strncmp(a->local, b->local, JID_PART_MAX_LEN);
        if (rv != 0) {
            return rv;
        }
    }

    if (a->domain == NULL) {
        if (b->domain != NULL && strncmp(b->domain, "*", JID_PART_MAX_LEN)
                != 0) {
            return 1;
        }
    }
    if (b->domain == NULL) {
        if (a->domain != NULL && strncmp(a->domain, "*", JID_PART_MAX_LEN)
                != 0) {
            return 1;
        }
    }
    if (a->domain != NULL && b->domain != NULL
            && strncmp(a->domain, "*", JID_PART_MAX_LEN) != 0
            && strncmp(b->domain, "*", JID_PART_MAX_LEN) != 0) {
        int rv = strncmp(a->domain, b->domain, JID_PART_MAX_LEN);
        if (rv != 0) {
            return rv;
        }
    }

    /* If one resource is NULL, and the other isn't we don't care.  Only if
     * they both do, and they are different. */
    if (a->resource != NULL && b->resource != NULL
            && strncmp(a->resource, "*", JID_PART_MAX_LEN) != 0
            && strncmp(b->resource, "*", JID_PART_MAX_LEN) != 0) {
        return strncmp(a->resource, b->resource, JID_PART_MAX_LEN);
    }
    return 0;
}

const char* jid_local(const struct jid *jid) {
    return jid->local;
}

void jid_set_local(struct jid *jid, const char *localpart) {
    copy_string(&jid->local, localpart);
}

const char* jid_domain(const struct jid *jid) {
    return jid->domain;
}

void jid_set_domain(struct jid *jid, const char *domainpart) {
    copy_string(&jid->domain, domainpart);
}

const char* jid_resource(const struct jid *jid) {
    return jid->resource;
}

void jid_set_resource(struct jid *jid, const char *resourcepart) {
    copy_string(&jid->resource, resourcepart);
}
