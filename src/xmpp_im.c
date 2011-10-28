/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "log.h"
#include "utils.h"
#include "xmpp.h"
#include "xmpp_common.h"
#include "xep_disco.h"

/* This should probably go in the utils module, if I use it outside of here,
 * I will. */
#define ALLOC_COPY_STRING(a, b) a = calloc(strlen(b) + 1, sizeof(char)); \
                                check_mem(a); \
                                strcpy(a, b)

// XML String constants
static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

static const char *MSG_ROSTER =
    "<iq id='%s' type='result'>"
        "<query xmlns='jabber:iq:roster'>"
        "</query>"
    "</iq>";

static const char *MSG_MESSAGE_START =
    "<message from='%s' to='%s' type='chat'><body>";
static const char *MSG_MESSAGE_END =
    "</body></message>";

// Forward declarations
static void stanza_start(void *data, const char *name, const char **attrs);
static void stanza_data(void *data, const char *s, int len);

static void handle_message(struct stanza_info *stanza_info,
                           const char **attrs);
static void message_start(void *data, const char *name, const char **attrs);
static void message_end(void *data, const char *name);

static void handle_presence(struct stanza_info *stanza_info,
                            const char **attrs);
static void presence_start(void *data, const char *name, const char **attrs);
static void presence_end(void *data, const char *name);

static void handle_iq(struct stanza_info *stanza_info,
                      const char **attrs);
static void iq_start(void *data, const char *name, const char **attrs);
static void iq_end(void *data, const char *name);

static void new_stanza_info_jid(struct jid *jid, const char *strjid);

static struct stanza_info* new_stanza_info(struct client_info *info,
                                           const char *name,
                                           const char **attrs);

static void del_stanza_info(struct stanza_info *stanza_info);

static bool iq_session(struct stanza_info *stanza_info, const char *name,
                       const char **attrs);
static void iq_session_end(void *data, const char *name);

static bool iq_roster_query(struct stanza_info *stanza_info, const char *name,
                            const char **attrs);
static void iq_roster_query_end(void *data, const char *name);

static bool message_body(struct stanza_info *stanza_info, const char *name,
                         const char **attrs);
static void message_body_end(void *data, const char *name);
static void message_body_data(void *data, const char *s, int len);

static struct xep_iq_namespace {
    const char *namespace;
    xmpp_stanza_handler handler;
} IQ_NAMESPACES[] = {
    {XMPP_NS_SESSION, iq_session},
    {XMPP_NS_ROSTER, iq_roster_query},
    {XMPP_NS_DISCO_ITEMS, disco_query_items},
    {XMPP_NS_DISCO_INFO, disco_query_info},
};

void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stanza_start, xmpp_stream_end);
    XML_SetCharacterDataHandler(parser, xmpp_ignore_data);
}

void xmpp_im_stanza_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *info = stanza_info->info;

    if (strcmp(stanza_info->name, name) == 0) {
        log_info("Stanza end");
        xmpp_print_end_tag(name);
        XML_SetUserData(info->parser, info);
        xmpp_im_set_handlers(info->parser);
        del_stanza_info(stanza_info);
    } else {
        log_warn("Unhandled stanza end");
        xmpp_print_end_tag(name);
    }
}

static void stanza_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Stanza start");
    xmpp_print_start_tag(name, attrs);

    struct stanza_info *stanza_info = new_stanza_info(info, name, attrs);
    XML_SetUserData(info->parser, stanza_info);

    XML_SetEndElementHandler(info->parser, xmpp_im_stanza_end);
    XML_SetCharacterDataHandler(info->parser, stanza_data);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        handle_message(stanza_info, attrs);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        handle_presence(stanza_info, attrs);
    } else if (strcmp(name, XMPP_IQ) == 0) {
        handle_iq(stanza_info, attrs);
    }
}

static void stanza_data(void *data, const char *s, int len) {
    log_warn("Unhandled main stanza data");
    xmpp_print_data(s, len);
}

static void handle_message(struct stanza_info *stanza_info,
                           const char **attrs) {
    struct client_info *info = stanza_info->info;
    XML_SetElementHandler(info->parser, message_start, message_end);
}

static void message_start(void *data, const char *name, const char **attrs) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    //struct client_info *info = stanza_info->info;

    log_info("Inner message start");
    xmpp_print_start_tag(name, attrs);

    if (strcmp(name, XMPP_MESSAGE_BODY) == 0) {
        message_body(stanza_info, name, attrs);
    } else {
        // ignore...
    }
}

static void message_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *client_info = stanza_info->info;

    log_info("Inner message end");
    xmpp_print_end_tag(name);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        struct client_info *to_info = xmpp_find_client(&stanza_info->to,
                client_info->server_info);
        if (to_info == NULL) {
            // TODO: send error message instead of bailing
            log_warn("Destination not connected");
            goto error;
        }
        check(sendall(to_info->fd, MSG_MESSAGE_END,
                      strlen(MSG_MESSAGE_END)) > 0,
              "Error sending message to destination");
        xmpp_im_stanza_end(data, name);
    }
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client_info->parser, false);
}

static void handle_presence(struct stanza_info *stanza_info,
                            const char **attrs) {
    struct client_info *info = stanza_info->info;
    XML_SetElementHandler(info->parser, presence_start, presence_end);
}

static void presence_start(void *data, const char *name, const char **attrs) {
    log_info("Inner presence start");
    xmpp_print_start_tag(name, attrs);
    // ignore
}

static void presence_end(void *data, const char *name) {
    //struct stanza_info *stanza_info = (struct stanza_info*)data;
    //struct client_info *client_info = stanza_info->info;

    log_info("Inner presence end");
    xmpp_print_end_tag(name);

    if (strcmp(name, XMPP_PRESENCE) == 0) {
        xmpp_im_stanza_end(data, name);
    }
}

static void handle_iq(struct stanza_info *stanza_info,
                      const char **attrs) {
    struct client_info *info = stanza_info->info;
    XML_SetElementHandler(info->parser, iq_start, iq_end);
}

static void iq_start(void *data, const char *name, const char **attrs) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    //struct client_info *info = stanza_info->info;

    log_info("Inner IQ start");
    xmpp_print_start_tag(name, attrs);

    if (stanza_info->is_unhandled) {
        /* We are not going to magically start handling an IQ block
         * mid-parse. */
        return;
    }

    int i;
    const int end = sizeof(IQ_NAMESPACES) / sizeof(struct xep_iq_namespace);
    for (i = 0; i < end; i++) {
        /* name is "namespace tag_name", a space inbetween.  We only want to
         * compare namespaces. */
        int ns_len = strchr(name, ' ') - name;
        if (strncmp(name, IQ_NAMESPACES[i].namespace, ns_len) == 0) {
            if (IQ_NAMESPACES[i].handler(stanza_info, name, attrs)) {
                return;
            }
        }
    }

    // If we get here, we haven't handled the stanza
    log_warn("Unhandled iq start");
    stanza_info->is_unhandled = true;
}

static void iq_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    //struct client_info *client_info = stanza_info->info;

    log_info("Inner IQ end");
    xmpp_print_end_tag(name);

    if (strcmp(name, XMPP_IQ) == 0) {
        if (stanza_info->is_unhandled && stanza_info->id != NULL) {
            xmpp_send_not_supported(stanza_info);
        }
        xmpp_im_stanza_end(data, name);
    }
}

static void new_stanza_info_jid(struct jid *jid, const char *strjid) {
    char *domainstr = strchr(strjid, '@');
    char *resourcestr = strchr(strjid, '/');

    int locallen;
    if (domainstr == NULL) {
        locallen = strlen(strjid);
    } else {
        locallen = domainstr - strjid;
    }

    jid->local = calloc(1, locallen);
    check_mem(jid->local);
    strncpy(jid->local, strjid, locallen);

    if (domainstr == NULL) {
        return;
    } else {
        domainstr++; // was previously on the '@' character in the string
    }

    int domainlen;
    if (resourcestr == NULL) {
        domainlen = strlen(domainstr);
    } else {
        domainlen = resourcestr - domainstr;
    }
    jid->domain = calloc(1, domainlen);
    check_mem(jid->domain);
    strncpy(jid->domain, domainstr, domainlen);

    if (resourcestr == NULL) {
        return;
    } else {
        resourcestr++; // was previously on the '/' character
    }

    jid->resource = calloc(1, strlen(resourcestr));
    check_mem(jid->resource);
    strncpy(jid->resource, resourcestr, strlen(resourcestr));
}

static struct stanza_info* new_stanza_info(struct client_info *info,
                                           const char *name,
                                           const char **attrs) {
    struct stanza_info *stanza_info = calloc(1, sizeof(*stanza_info));
    check_mem(stanza_info);
    stanza_info->is_unhandled = false;
    stanza_info->info = info;

    ALLOC_COPY_STRING(stanza_info->name, name);
    for (int i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_ID) == 0) {
            ALLOC_COPY_STRING(stanza_info->id, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TO) == 0) {
            new_stanza_info_jid(&stanza_info->to, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_FROM) == 0) {
            new_stanza_info_jid(&stanza_info->from, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            ALLOC_COPY_STRING(stanza_info->type, attrs[i + 1]);
        }
    }
    return stanza_info;
}

static void del_stanza_info(struct stanza_info *stanza_info) {
    free(stanza_info->name);
    free(stanza_info->id);
    free(stanza_info->to.local);
    free(stanza_info->to.domain);
    free(stanza_info->to.resource);
    free(stanza_info->from.local);
    free(stanza_info->from.domain);
    free(stanza_info->from.resource);
    free(stanza_info->type);
    free(stanza_info);
}

static bool iq_session(struct stanza_info *stanza_info, const char *name,
                       const char **attrs) {
    struct client_info *info = stanza_info->info;

    if (strcmp(stanza_info->name, XMPP_IQ) != 0) {
        return false;
    }

    log_info("Session IQ Start");

    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");
    check(stanza_info->id != NULL, "Session IQ needs id attribute");
    check(strcmp(stanza_info->type, XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    XML_SetEndElementHandler(info->parser, iq_session_end);
    return true;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(info->parser, false);
    return false;
}

static void iq_session_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *info = stanza_info->info;

    char msg[strlen(MSG_STREAM_SUCCESS) + strlen(stanza_info->id)];

    log_info("Session IQ End");
    xmpp_print_end_tag(name);

    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_STREAM_SUCCESS, stanza_info->id);
    check(sendall(info->fd, msg, strlen(msg)) > 0,
          "Error sending stream success message");

    XML_SetEndElementHandler(info->parser, xmpp_im_stanza_end);
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(info->parser, false);
}

static bool iq_roster_query(struct stanza_info *stanza_info, const char *name,
                            const char **attrs) {
    struct client_info *info = stanza_info->info;

    if (strcmp(stanza_info->name, XMPP_IQ) != 0) {
        return false;
    }

    log_info("Roster Query IQ Start");

    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");
    check(strcmp(stanza_info->type, XMPP_ATTR_TYPE_GET) == 0,
          "Session IQ type must be \"set\"");

    XML_SetEndElementHandler(info->parser, iq_roster_query_end);
    return true;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(info->parser, false);
    return false;
}

static void iq_roster_query_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *info = stanza_info->info;

    char msg[strlen(MSG_ROSTER) + strlen(stanza_info->id)];

    log_info("Roster Query IQ End");
    xmpp_print_end_tag(name);
    check(strcmp(name, XMPP_IQ_QUERY_ROSTER) == 0, "Unexpected stanza");

    snprintf(msg, sizeof(msg), MSG_ROSTER, stanza_info->id);
    check(sendall(info->fd, msg, strlen(msg)) > 0,
          "Error sending roster message");

    XML_SetEndElementHandler(info->parser, xmpp_im_stanza_end);
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(info->parser, false);
}

static bool message_body(struct stanza_info *stanza_info, const char *name,
                         const char **attrs) {
    struct client_info *client_info = stanza_info->info;

    if (strcmp(stanza_info->name, XMPP_MESSAGE) != 0) {
        return false;
    }
    if (strcmp(name, XMPP_MESSAGE_BODY) != 0) {
        /* TODO: We only care about body messages right now.  This catches
         * and ignores any sub-stanzas of a message stanza, which is probably
         * bad. */
        return true;
    }
    if (stanza_info->to.local == NULL) {
        log_warn("Stanza doesn't have a \"to\" field, ignoring");
        return false;
    }

    log_info("Message body start");

    struct client_info *to_info = xmpp_find_client(&stanza_info->to,
                                                   client_info->server_info);

    char *fromstr = jid_to_str(&client_info->jid);
    char *tostr = "";
    if (to_info != NULL) {
        tostr = jid_to_str(&to_info->jid);
    }

    char msg[strlen(MSG_MESSAGE_START) + strlen(fromstr) + strlen(tostr)];

    if (to_info == NULL) {
        // TODO: send error message instead of bailing
        log_warn("Destination not connected");
        goto error;
    }

    snprintf(msg, sizeof(msg), MSG_MESSAGE_START, fromstr, tostr);
    check(sendall(to_info->fd, msg, strlen(msg)) > 0,
          "Error sending message to destination");


    XML_SetEndElementHandler(client_info->parser, message_body_end);
    XML_SetCharacterDataHandler(client_info->parser, message_body_data);
    return true;

error:
    /* TODO: Rework this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client_info->parser, false);
    return false;
}

static void message_body_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *client_info = stanza_info->info;

    log_info("Message body end");
    xmpp_print_end_tag(name);

    XML_SetEndElementHandler(client_info->parser, message_end);
    return;
}

static void message_body_data(void *data, const char *s, int len) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *client_info = stanza_info->info;

    log_info("Message body data");
    xmpp_print_data(s, len);

    struct client_info *to_info = xmpp_find_client(&stanza_info->to,
                                                   client_info->server_info);
    if (to_info == NULL) {
        // TODO: send error message instead of bailing
        log_warn("Destination not connected");
        goto error;
    }
    check(sendall(to_info->fd, s, len) > 0,
          "Error sending message to destination");
    return;

error:
    /* TODO: Rework this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(client_info->parser, false);
}
