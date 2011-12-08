/**
 * xmp3 - XMPP Proxy
 * jid.{c,h} - Functions and data structure to manipulate a JID
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "utstring.h"

#include "log.h"
#include "utils.h"

#include "jid.h"

#define MAX_JIDSTR_LEN 3071

// Forward declarations
static void copy_jid_field(char *jidfield, const char *arg);

/** Represents a JID (local@domain/resource). */
struct jid {
    char *local;
    char *domain;
    char *resource;
};

struct jid* jid_new() {
    struct jid *jid =  calloc(1, sizeof(*jid));
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

    /* We use this temp string to replace delimiters with NULLs, then copy
     * those substrings out into the final structure. */
    char *tmpstr;
    STRNDUP_CHECK(tmpstr, jidstr, MAX_JIDSTR_LEN);

    char *at_delim = strchr(tmpstr, '@');
    if (at_delim != NULL) {
        *at_delim = '\0';
        at_delim++;
    }

    char *slash_delim = strchr(tmpstr, '/');
    if(slash_delim != NULL) {
        *slash_delim = '\0';
        slash_delim++;
    }

    if (at_delim == NULL) {
        jid_set_domain(jid, tmpstr);
    } else {
        jid_set_local(jid, tmpstr);
        jid_set_domain(jid, at_delim);
    }

    if (slash_delim != NULL) {
        jid_set_resource(jid, slash_delim);
    }

    free(tmpstr);
    return jid;
}

struct jid* jid_new_from_jid(const struct jid *jid) {
    struct jid *newjid = jid_new();
    jid_set_local(newjid, jid_local(jid));
    jid_set_domain(newjid, jid_local(jid));
    jid_set_resource(newjid, jid_local(jid));

    return newjid;
}

char* jid_to_str(const struct jid *jid) {
    // Domain part is required
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

    return utstring_body(&strjid);
}

int jid_to_str_len(const struct jid *jid) {
    int len = strlen(jid->domain);
    if (jid->local) {
        // +1 for '@'
        len += strlen(jid->local) + 1;
    }
    if (jid->resource) {
        // +1 for '/'
        len += strlen(jid->resource) + 1;
    }
    return len;
}

int jid_cmp(const struct jid *a, const struct jid *b) {
    // If one has a local part, and the other doesn't then no match.
    if ((a->local == NULL) != (b->local == NULL)) {
        return a->local == NULL ? -1 : 1;
    }

    if (a->local != NULL && b->local != NULL) {
        int rv = strcmp(a->local, b->local);
        if (rv != 0) {
            return rv;
        }
    }

    if ((a->domain == NULL) != (b->domain == NULL)) {
        return a->domain == NULL ? -1 : 1;
    }

    if (a->domain != NULL && b->domain != NULL) {
        int rv = strcmp(a->domain, b->domain);
        if (rv != 0) {
            return rv;
        }
    }

    if ((a->resource == NULL) != (b->resource == NULL)) {
        return a->domain == NULL ? -1 : 1;
    }

    if (a->domain != NULL && b->domain != NULL) {
        return strcmp(a->resource, b->resource);
    }

    return 0;
}

const char* jid_local(const struct jid *jid) {
    return jid->local;
}

void jid_set_local(struct jid *jid, const char *localpart) {
    copy_jid_field(jid->local, localpart);
}

const char* jid_domain(const struct jid *jid) {
    return jid->domain;
}

void jid_set_domain(struct jid *jid, const char *domainpart) {
    copy_jid_field(jid->domain, domainpart);
}

const char* jid_resource(const struct jid *jid) {
    return jid->resource;
}

void jid_set_resource(struct jid *jid, const char *resourcepart) {
    copy_jid_field(jid->resource, resourcepart);
}

static void copy_jid_field(char *jidfield, const char *arg) {
    free(jidfield);
    if (arg == NULL) {
        jidfield = NULL;
    } else {
        STRNDUP_CHECK(jidfield, arg, MAX_JIDSTR_LEN);
    }
}
