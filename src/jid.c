/**
 * xmp3 - XMPP Proxy
 * jid.{c,h} - Functions and data structure to manipulate a JID
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "jid.h"

#include "utstring.h"

#include "log.h"
#include "utils.h"

#define MAX_JIDSTR_LEN 3071

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
    char *tmpstr = strndupa(jidstr, MAX_JIDSTR_LEN);

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
    return jid;
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

const char* jid_local(const struct jid *jid) {
    return jid->local;
}

void jid_set_local(struct jid *jid, const char *localpart) {
    free(jid->local);
    STRNDUP_CHECK(jid->local, localpart, MAX_JIDSTR_LEN);
}

const char* jid_domain(const struct jid *jid) {
    return jid->domain;
}

void jid_set_domain(struct jid *jid, const char *domainpart) {
    free(jid->domain);
    STRNDUP_CHECK(jid->domain, domainpart, MAX_JIDSTR_LEN);
}

const char* jid_resource(const struct jid *jid) {
    return jid->resource;
}

void jid_set_resource(struct jid *jid, const char *domainpart) {
    free(jid->domain);
    STRNDUP_CHECK(jid->domain, domainpart, MAX_JIDSTR_LEN);
}
