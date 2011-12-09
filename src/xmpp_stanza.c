/**
 * xmp3 - XMPP Proxy
 * xmpp_stanza.{c,h} - Represents top-level XMPP stanzas received from clients.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <stdlib.h>

#include "uthash.h"
#include "utstring.h"

#include "log.h"
#include "utils.h"

#include "jid.h"
#include "xmpp_client.h"
#include "xmpp_common.h"
#include "xmpp_server.h"

#include "xmpp_stanza.h"

const char *XMPP_STANZA_ATTR_TO = "to";
const char *XMPP_STANZA_ATTR_FROM = "from";
const char *XMPP_STANZA_ATTR_ID = "id";
const char *XMPP_STANZA_ATTR_TYPE = "type";

const char *XMPP_ATTR_TYPE_GET = "get";
const char *XMPP_ATTR_TYPE_SET = "set";
const char *XMPP_ATTR_TYPE_RESULT = "result";
const char *XMPP_ATTR_TYPE_ERROR = "error";


// Forward declarations
static struct jid* copy_attr_jid(const struct xmpp_stanza *stanza,
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

    /** The value of the "to" attribute, as a JID structure. */
    struct jid *to_jid;

    /** The value of the "from" attribute, as a JID structure. */
    struct jid *from_jid;

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

    // If there is no "from" attribute, add one.
    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    if (from == NULL) {
        struct attribute *attr = calloc(1, sizeof(*attr));
        STRDUP_CHECK(attr->name, XMPP_STANZA_ATTR_FROM);
        attr->value = jid_to_str(xmpp_client_jid(client));
        check_mem(attr->value);
        HASH_ADD_KEYPTR(hh, stanza->attributes, attr->name, strlen(attr->name),
                        attr);
    }

    // If there is no "to" attribute, add one.
    const char *to = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TO);
    if (to == NULL) {
        struct attribute *attr = calloc(1, sizeof(*attr));
        STRDUP_CHECK(attr->name, XMPP_STANZA_ATTR_TO);

        if (strcmp(stanza->ns_name, XMPP_MESSAGE) == 0) {
            /* If there is no "to" in a message, it is addressed to the bare
             * JID of the sender. */
            struct jid *jid = jid_new_from_jid(xmpp_client_jid(client));
            check_mem(jid);
            jid_set_resource(jid, NULL);
            attr->value = jid_to_str(jid);
            check_mem(attr->value);
            free(jid);

        } else {
            /* Else it is addressed to the server. */
            attr->value = jid_to_str(xmpp_server_jid(
                                     xmpp_client_server(client)));
            check_mem(attr->value);
        }
        HASH_ADD_KEYPTR(hh, stanza->attributes, attr->name, strlen(attr->name),
                        attr);
    }

    // Create JID structures from the "to" and "from" attributes
    stanza->to_jid = copy_attr_jid(stanza, XMPP_STANZA_ATTR_TO);
    stanza->from_jid = copy_attr_jid(stanza, XMPP_STANZA_ATTR_FROM);
    return stanza;
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

const char* xmpp_stanza_ns_name(const struct xmpp_stanza *stanza) {
    return stanza->ns_name;
}

struct jid* xmpp_stanza_jid_from(const struct xmpp_stanza *stanza) {
    return stanza->from_jid;
}

struct jid* xmpp_stanza_jid_to(const struct xmpp_stanza *stanza) {
    return stanza->to_jid;
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

void xmpp_stanza_set_attr(struct xmpp_stanza *stanza, const char *name,
                          const char *value) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);

    if (attr == NULL) {
        if (value == NULL) {
            return;
        }
        attr = calloc(1, sizeof(*attr));
        check_mem(attr);
        attr->name = strdup(name);
        check_mem(attr->name);
        HASH_ADD_KEYPTR(hh, stanza->attributes, name, strlen(name), attr);
    } else {
        if (value == NULL) {
            HASH_DEL(stanza->attributes, attr);
            free(attr);
            return;
        }
        free(attr->value);
    }
    attr->value = strdup(value);
    check_mem(attr->value);
}

static struct jid* copy_attr_jid(const struct xmpp_stanza *stanza,
                                       const char *name) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);
    if (attr == NULL) {
        return NULL;
    } else {
        return jid_new_from_str(attr->value);
    }
}

char* xmpp_stanza_create_tag(const struct xmpp_stanza *stanza) {
    UT_string s;
    utstring_init(&s);

    utstring_printf(&s, "<%s", stanza->name);

    if (stanza->namespace != NULL) {
        utstring_printf(&s, " xmlns='%s'", stanza->namespace);
    }

    struct attribute *attr, *attr_tmp;
    HASH_ITER(hh, stanza->attributes, attr, attr_tmp) {
        utstring_printf(&s, " %s='%s'", attr->name, attr->value);
    }

    utstring_printf(&s, ">");

    return utstring_body(&s);
}
