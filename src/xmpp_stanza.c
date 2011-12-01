/**
 * xmp3 - XMPP Proxy
 * xmpp_stanza.{c,h} - Represents top-level XMPP stanzas received from clients.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xmpp_stanza.h"

#include <stdlib.h>

#include "uthash.h"
#include "utils.h"

const char *XMPP_STANZA_ATTR_TO = "to";
const char *XMPP_STANZA_ATTR_FROM = "from";
const char *XMPP_STANZA_ATTR_ID = "id";
const char *XMPP_STANZA_ATTR_TYPE = "type";

// Forward declarations
static const struct jid* copy_attr_jid(const struct xmpp_stanza *stanza,
                                       const char *name);

struct attribute {
    char *name;
    char *value;

    /** These are stored in a hash table. */
    UT_hash_handle hh;
};

struct xmpp_stanza {
    /** The client who sent this stanza. */
    struct xmpp_client *client;

    /** The name + namespace string. */
    char *ns_name;

    /** The name of this tag. */
    char *name;

    /** The namespace of this tag. */
    char *namespace;

    /** A hash table of stanza attributes. */
    struct attribute *attributes;
};

struct xmpp_stanza* xmpp_stanza_new(struct xmpp_client *client,
                                    const char *name, const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);

    stanza->client = client;

    STRDUP_CHECK(stanza->ns_name, name);

    char *ns_delim = strrchr(name, *XMPP_NS_SEPARATOR);
    if (ns_delim == NULL) {
        STRDUP_CHECK(stanza->name, name);
    } else {
        int ns_len = ns_delim - name;
        STRNDUP_CHECK(stanza->namespace, name, ns_len);
        STRDUP_CHECK(stanza->name, ns_delim + 1);
    }

    for (int i = 0; attrs[i] != NULL; i += 2) {
        struct attribute *attr = calloc(1, sizeof(*attr));
        STRDUP_CHECK(attr->name, attrs[i]);
        STRDUP_CHECK(attr->value, attrs[i + 1]);
        HASH_ADD_KEYPTR(hh, stanza->attributes, attr->name, strlen(attr->name),
                        attr);
    }
}

void xmpp_stanza_del(struct xmpp_stanza *stanza) {
    free(stanza->ns_name);
    free(stanza->name);
    free(stanza->namespace);

    struct attribute *attr, *tmp;
    HASH_ITER(hh, stanza->attributes, attr, tmp) {
        HASH_DEL(stanza->attributes, attr);
        free(attr);
    }

    free(stanza);
}

struct xmpp_client* xmpp_stanza_client(const struct xmpp_stanza *stanza) {
    return stanza->client;
}

const char* xmpp_stanza_name(const struct xmpp_stanza *stanza) {
    return stanza->name;
}

const char* xmpp_stanza_namespace(const struct xmpp_stanza *stanza) {
    return stanza->namespace;
}

const char* xmpp_stanza_name_namespace(const struct xmpp_stanza *stanza) {
    return stanza->ns_name;
}

struct jid* xmpp_stanza_jid_from(const struct xmpp_stanza *stanza) {
    return copy_attr_jid(stanza, XMPP_STANZA_FROM);
}

struct jid* xmpp_stanza_jid_to(const struct xmpp_stanza *stanza) {
    return copy_attr_jid(stanza, XMPP_STANZA_TO);
}

const char* xmpp_stanza_attr(const struct xmpp_stanza *stanza,
                             const char *name) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);
    if (attr == NULL) {
        return NULL;
    } else {
        return attr->value;
    }
}

static const struct jid* copy_attr_jid(const struct xmpp_stanza *stanza,
                                       const char *name) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);
    if (attr == NULL) {
        return NULL;
    } else {
        return jid_new_from_str(attr->value);
    }
}
