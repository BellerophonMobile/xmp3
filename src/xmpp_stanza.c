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
 * @file xmpp_stanza.c
 * Represents top-level XMPP stanzas received from clients.
 */

#include <stdlib.h>

#include <uthash.h>
#include <utlist.h>
#include <utstring.h>

#include "log.h"
#include "utils.h"
#include "xmpp_parser.h"

#include "xmpp_stanza.h"

const char *XMPP_STANZA_NS_CLIENT = "jabber:client";
const char *XMPP_STANZA_NS_STANZA = "urn:ietf:params:xml:ns:xmpp-stanzas";

const char *XMPP_STANZA_MESSAGE = "message";
const char *XMPP_STANZA_PRESENCE = "presence";
const char *XMPP_STANZA_IQ = "iq";

const char *XMPP_STANZA_ATTR_TO = "to";
const char *XMPP_STANZA_ATTR_FROM = "from";
const char *XMPP_STANZA_ATTR_ID = "id";
const char *XMPP_STANZA_ATTR_TYPE = "type";

const char *XMPP_STANZA_TYPE_SET = "set";
const char *XMPP_STANZA_TYPE_GET = "get";
const char *XMPP_STANZA_TYPE_RESULT = "result";
const char *XMPP_STANZA_TYPE_ERROR = "error";

/** Structure to store a stanza attribute. */
struct attribute {
    /** Key for use in the hash table. */
    char *key;

    /** The name of this attribute. */
    char *name;

    /** The namespace URI of this attribute (may be NULL). */
    char *uri;

    /** The namespace prefix used on this attribute (may be null). */
    char *prefix;

    /** The value of this attribute. */
    char *value;

    /** These are stored in a hash table. */
    UT_hash_handle hh;
};

static void parse_ns(const char *ns_name, char **name, char **prefix, char **uri);
static void stanza_tostr(struct xmpp_stanza *stanza, UT_string *str);
static void attribute_del(struct attribute *attr);
static char* make_key(const char *name, const char *uri);

struct xmpp_stanza {
    /** The name of this tag. */
    char *name;

    /** The namespace URI of this tag. */
    char *uri;

    /** The namespace prefix of this tag. */
    char *prefix;

    /** A hash table of stanza attributes. */
    struct attribute *attributes;

    /** The data inside this node */
    UT_string data;

    /** A linked list of child nodes. */
    struct xmpp_stanza *children;

    /** Pointer to the parent stanza. */
    struct xmpp_stanza *parent;

    /** A list of namespaces defined with this stanza. */
    struct xmpp_parser_namespace *namespaces;

    /** Stanzas are kept in a linked list. */
    struct xmpp_stanza *prev;
    struct xmpp_stanza *next;
};

struct xmpp_stanza* xmpp_stanza_new(const char *ns_name, const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);

    parse_ns(ns_name, &stanza->name, &stanza->prefix, &stanza->uri);
    utstring_init(&stanza->data);

    if (attrs != NULL) {
        for (int i = 0; attrs[i] != NULL; i += 2) {
            struct attribute *attr = calloc(1, sizeof(*attr));
            check_mem(attr);

            parse_ns(attrs[i], &attr->name, &attr->prefix, &attr->uri);
            STRDUP_CHECK(attr->value, attrs[i + 1]);

            attr->key = make_key(attr->name, attr->uri);
            HASH_ADD_KEYPTR(hh, stanza->attributes, attr->key,
                            strlen(attr->key), attr);
        }
    }
    return stanza;
}

struct xmpp_stanza* xmpp_stanza_ns_new(const char *ns_name, const char **attrs,
                                    struct xmpp_parser_namespace *namespaces) {
    struct xmpp_stanza *stanza = xmpp_stanza_new(ns_name, attrs);
    stanza->namespaces = namespaces;
    return stanza;
}

void xmpp_stanza_del(struct xmpp_stanza *stanza, bool recursive) {
    if (stanza->uri) {
        free(stanza->uri);
    }
    if (stanza->prefix) {
        free(stanza->prefix);
    }
    free(stanza->name);

    struct attribute *attr, *tmp;
    HASH_ITER(hh, stanza->attributes, attr, tmp) {
        HASH_DEL(stanza->attributes, attr);
        attribute_del(attr);
    }

    if (stanza->namespaces) {
        xmpp_parser_namespace_del(stanza->namespaces);
    }
    utstring_done(&stanza->data);

    if (recursive) {
        struct xmpp_stanza *s, *tmp;
        DL_FOREACH_SAFE(stanza->children, s, tmp) {
            DL_DELETE(stanza->children, s);
            xmpp_stanza_del(s, true);
        }
    }

    free(stanza);
}

char* xmpp_stanza_string(struct xmpp_stanza *stanza, size_t *len) {
    UT_string str;
    utstring_init(&str);
    stanza_tostr(stanza, &str);
    if (len != NULL) {
        *len = utstring_len(&str);
    }
    return utstring_body(&str);
}

const char* xmpp_stanza_uri(const struct xmpp_stanza *stanza) {
    return stanza->uri;
}

void xmpp_stanza_copy_uri(struct xmpp_stanza *stanza, const char *uri) {
    copy_string(&stanza->uri, uri);
}

const char* xmpp_stanza_prefix(const struct xmpp_stanza *stanza) {
    return stanza->prefix;
}

void xmpp_stanza_copy_prefix(struct xmpp_stanza *stanza, const char *prefix) {
    copy_string(&stanza->prefix, prefix);
}

const char* xmpp_stanza_name(const struct xmpp_stanza *stanza) {
    return stanza->name;
}

void xmpp_stanza_copy_name(struct xmpp_stanza *stanza, const char *name) {
    copy_string(&stanza->name, name);
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

const char* xmpp_stanza_ns_attr(const struct xmpp_stanza *stanza,
                                const char *name, const char *uri) {
    char *key = make_key(name, uri);
    const char *value = xmpp_stanza_attr(stanza, key);
    free(key);
    return value;
}

void xmpp_stanza_set_attr(struct xmpp_stanza *stanza, const char *name,
                          char *value) {
    struct attribute *attr;
    HASH_FIND_STR(stanza->attributes, name, attr);

    if (attr == NULL) {
        /* If value is NULL, we want to delete an attribute that is already
         * gone. */
        if (value == NULL) {
            return;
        }
        attr = calloc(1, sizeof(*attr));
        check_mem(attr);
        STRDUP_CHECK(attr->name, name);
        HASH_ADD_KEYPTR(hh, stanza->attributes, name, strlen(name), attr);
    } else {
        /* If value is NULL, we want to delete this attribute. */
        if (value == NULL) {
            HASH_DEL(stanza->attributes, attr);
            attribute_del(attr);
            return;
        }
        free(attr->value);
    }
    attr->value = value;
}

void xmpp_stanza_set_ns_attr(struct xmpp_stanza *stanza, const char *name,
                            const char *uri, const char *prefix, char *value) {
    char *key = make_key(name, uri);
    xmpp_stanza_set_attr(stanza, key, value);
    if (value != NULL) {
        struct attribute *attr;
        HASH_FIND_STR(stanza->attributes, key, attr);
        if (attr == NULL) {
            log_err("Setting uri/prefix of nonexistant attribute, this shouldn't happen!");
            goto done;
        }
        if (uri) {
            STRDUP_CHECK(attr->uri, uri);
        }
        if (prefix) {
            STRDUP_CHECK(attr->prefix, prefix);
        }
    }
done:
    free(key);
}

void xmpp_stanza_copy_attr(struct xmpp_stanza *stanza, const char *name,
                           const char *value) {
    if (value == NULL) {
        xmpp_stanza_set_attr(stanza, name, NULL);
    } else {
        char *value_copy;
        STRDUP_CHECK(value_copy, value);
        xmpp_stanza_set_attr(stanza, name, value_copy);
    }
}

void xmpp_stanza_copy_ns_attr(struct xmpp_stanza *stanza, const char *name,
                              const char *uri, const char *prefix,
                              const char *value) {
    if (value == NULL) {
        xmpp_stanza_set_ns_attr(stanza, name, uri, prefix, NULL);
    } else {
        char *value_copy;
        STRDUP_CHECK(value_copy, value);
        xmpp_stanza_set_ns_attr(stanza, name, uri, prefix, value_copy);
    }
}

const char* xmpp_stanza_data(const struct xmpp_stanza *stanza) {
    return utstring_body(&stanza->data);
}

unsigned int xmpp_stanza_data_length(const struct xmpp_stanza *stanza) {
    return utstring_len(&stanza->data);
}

void xmpp_stanza_append_data(struct xmpp_stanza *stanza, const char *buf,
                             int len) {
    utstring_bincpy(&stanza->data, buf, len);
}

int xmpp_stanza_children_length(const struct xmpp_stanza *stanza) {
    int count = 0;
    struct xmpp_stanza *s;
    DL_FOREACH(stanza->children, s) {
        count++;
    }
    return count;
}

struct xmpp_stanza* xmpp_stanza_children(struct xmpp_stanza *stanza) {
    return stanza->children;
}

struct xmpp_stanza* xmpp_stanza_parent(struct xmpp_stanza *stanza) {
    return stanza->parent;
}

struct xmpp_stanza* xmpp_stanza_next(struct xmpp_stanza *stanza) {
    return stanza->next;
}

struct xmpp_stanza* xmpp_stanza_prev(struct xmpp_stanza *stanza) {
    return stanza->prev;
}

void xmpp_stanza_append_child(struct xmpp_stanza *stanza,
                              struct xmpp_stanza *child) {
    if (child->parent != NULL) {
        xmpp_stanza_remove_child(child->parent, child);
    }
    DL_APPEND(stanza->children, child);
    child->parent = stanza;
}

void xmpp_stanza_remove_child(struct xmpp_stanza *stanza,
                              struct xmpp_stanza *child) {
    DL_DELETE(stanza->children, child);
    child->parent = NULL;
}

/**
 * Parse the namespace triple that Expat gives us.
 *
 * Format is <URI> <NAME> <PREFIX>
 */
static void parse_ns(const char *ns_name, char **name, char **prefix,
                     char **uri) {
    char *separator = strchr(ns_name, XMPP_PARSER_SEPARATOR);
    if (separator == NULL) {
        /* No namespace or prefix. */
        STRDUP_CHECK(*name, ns_name);

    } else {
        /* There is a namespace URI. */
        STRNDUP_CHECK(*uri, ns_name, separator - ns_name);

        char *tmp = separator + 1;
        separator = strchr(tmp, XMPP_PARSER_SEPARATOR);
        if (separator == NULL) {
            /* There is no namespace prefix. */
            STRDUP_CHECK(*name, tmp);
        } else {
            /* There is a namespace prefix. */
            STRNDUP_CHECK(*name, tmp, separator - tmp);
            STRDUP_CHECK(*prefix, separator + 1);
        }
    }
}

/**
 * Function that gets called recursively to convert a stanza to a string.
 *
 * Also prints child stanzas.
 */
static void stanza_tostr(struct xmpp_stanza *stanza, UT_string *str) {
    if (stanza->prefix) {
        utstring_printf(str, "<%s:%s", stanza->prefix, stanza->name);
    } else {
        utstring_printf(str, "<%s", stanza->name);
    }

    struct xmpp_parser_namespace *ns = stanza->namespaces;
    while (ns != NULL) {
        const char *prefix = xmpp_parser_namespace_prefix(ns);
        const char *uri = xmpp_parser_namespace_uri(ns);
        if (prefix) {
            utstring_printf(str, " xmlns:%s='%s'", prefix, uri);
        } else {
            utstring_printf(str, " xmlns='%s'", uri);
        }
        ns = xmpp_parser_namespace_next(ns);
    }

    struct attribute *attr, *tmp;
    HASH_ITER(hh, stanza->attributes, attr, tmp) {
        char quot;
        if (strchr(attr->value, '\'') != NULL) {
            quot = '"';
        } else {
            quot = '\'';
        }
        if (attr->prefix) {
            utstring_printf(str, " %s:%s=%c%s%c", attr->prefix, attr->name,
                            quot, attr->value, quot);
        } else {
            utstring_printf(str, " %s=%c%s%c", attr->name, quot, attr->value,
                            quot);
        }
    }

    if (stanza->children != NULL || utstring_len(&stanza->data) > 0) {
        utstring_printf(str, ">");
        utstring_concat(str, &stanza->data);
        struct xmpp_stanza *child;
        DL_FOREACH(stanza->children, child) {
            stanza_tostr(child, str);
        }
        if (stanza->prefix) {
            utstring_printf(str, "</%s:%s>", stanza->prefix, stanza->name);
        } else {
            utstring_printf(str, "</%s>", stanza->name);
        }
    } else {
        utstring_printf(str, "/>");
    }
}

/**
 * Convenience function to build a key for the attribute hash table.
 *
 * This is to take care of attributes that have namespace qualifiers.
 */
static char* make_key(const char *name, const char *uri) {
    int name_len = strlen(name);

    /* +1 for null terminator. */
    int key_len = name_len + 1;

    int uri_len = 0;
    if (uri != NULL) {
        uri_len = strlen(uri);
        /* +1 for space separator. */
        key_len += uri_len + 1;
    }

    char *key = malloc(key_len * sizeof(char));
    check_mem(key);

    if (uri != NULL) {
        memcpy(key, uri, uri_len);
        key[uri_len] = XMPP_PARSER_SEPARATOR;
        memcpy(key + uri_len + 1, name, name_len);
    } else {
        memcpy(key, name, name_len);
    }
    key[key_len - 1] = '\0';
    return key;
}

static void attribute_del(struct attribute *attr) {
    if (attr->uri) {
        free(attr->uri);
    }
    if (attr->prefix) {
        free(attr->prefix);
    }
    free(attr->name);
    free(attr->value);
    free(attr->key);
    free(attr);
}
