/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im.h"

#include <expat.h>

#include "log.h"
#include "utils.h"
#include "xmpp_common.h"

/* This should probably go in the utils module, if I use it outside of here,
 * I will. */
#define ALLOC_COPY_STRING(a, b) a = calloc(strlen(b) + 1, sizeof(char)); \
                                check_mem(a); \
                                strcpy(a, b)

// XML String constants
static const char *MSG_NOT_IMPLEMENTED =
    "<%s xmlns='%s' type='error' id='%s'"
            " from='localhost' to='%s@localhost/%s'>"
        "<error type='cancel'>"
            "<feature-not-implemented "
                "xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
        "</error>"
    "</%1$s>";

static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

static const char *MSG_ROSTER =
    "<iq id='%s' type='result'>"
        "<query xmlns='jabber:iq:roster'>"
        "</query>"
    "</iq>";

// Forward declarations
static void stanza_start(void *data, const char *name, const char **attrs);
static void stanza_end(void *data, const char *name);
static void stanza_data(void *data, const char *s, int len);

static void inner_stanza_start(void *data, const char *name,
                               const char **attrs);

static struct stanza_info* new_stanza_info(struct client_info *info,
                                           const char *name,
                                           const char **attrs);

static void del_stanza_info(struct stanza_info *stanza_info);

static void send_not_supported(struct stanza_info *stanza_info);

static bool iq_session(struct stanza_info *stanza_info, const char *name,
                       const char **attrs);
static void iq_session_end(void *data, const char *name);

static bool iq_roster_query(struct stanza_info *stanza_info, const char *name,
                            const char **attrs);
static void iq_roster_query_end(void *data, const char *name);

static struct xep_namespaces {
    const char *namespace;
    xmpp_stanza_handler handler;
} NAMESPACES[] = {
    {XMPP_NS_SESSION, iq_session},
    {XMPP_NS_ROSTER, iq_roster_query},
};

void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stanza_start, xmpp_stream_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void stanza_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Stanza start");
    xmpp_print_start_tag(name, attrs);

    struct stanza_info *stanza_info = new_stanza_info(info, name, attrs);

    XML_SetElementHandler(info->parser, inner_stanza_start, stanza_end);
    XML_SetCharacterDataHandler(info->parser, stanza_data);
    XML_SetUserData(info->parser, stanza_info);
}

static void stanza_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *info = stanza_info->info;

    if (strcmp(stanza_info->name, name) == 0) {
        log_info("Stanza end");
        xmpp_print_end_tag(name);
        // We've reached the end of this message/presence/iq block
        if (stanza_info->is_unhandled && stanza_info->id != NULL) {
            send_not_supported(stanza_info);
        }
        XML_SetUserData(info->parser, info);
        xmpp_im_set_handlers(info->parser);
        del_stanza_info(stanza_info);
    } else {
        log_warn("Unhandled stanza end");
        xmpp_print_end_tag(name);
    }
}

static void stanza_data(void *data, const char *s, int len) {
    log_warn("Unhandled main stanza data");
    xmpp_print_data(s, len);
}

static void inner_stanza_start(void *data, const char *name,
                               const char **attrs) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *info = stanza_info->info;

    log_info("Inner stanza start");
    xmpp_print_start_tag(name, attrs);

    if (stanza_info->is_unhandled) {
        /* We are not going to magically start handling a stanza block
         * mid-parse. */
        return;
    }

    int i;
    const int end = sizeof(NAMESPACES) / sizeof(struct xep_namespaces);
    for (i = 0; i < end; i++) {
        /* name is "namespace tag_name", a space inbetween.  We only want to
         * compare namespaces. */
        int ns_len = strchr(name, ' ') - name;
        if (strncmp(name, NAMESPACES[i].namespace, ns_len) == 0) {
            if (NAMESPACES[i].handler(stanza_info, name, attrs)) {
                return;
            }
        }
    }

    // If we get here, we haven't handled the stanza
    log_warn("Unhandled stanza start");
    stanza_info->is_unhandled = true;
    XML_SetEndElementHandler(info->parser, stanza_end);
    XML_SetCharacterDataHandler(info->parser, stanza_data);
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
            ALLOC_COPY_STRING(stanza_info->to, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_FROM) == 0) {
            ALLOC_COPY_STRING(stanza_info->from, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            ALLOC_COPY_STRING(stanza_info->type, attrs[i + 1]);
        }
    }
    return stanza_info;
}

static void del_stanza_info(struct stanza_info *stanza_info) {
    free(stanza_info->name);
    free(stanza_info->id);
    free(stanza_info->to);
    free(stanza_info->from);
    free(stanza_info->type);
    free(stanza_info);
}

static void send_not_supported(struct stanza_info *stanza_info) {
    struct client_info *info = stanza_info->info;
    char tag_name_buffer[strlen(stanza_info->name)];
    strcpy(tag_name_buffer, stanza_info->name);

    char *tag_name = tag_name_buffer;
    char *tag_ns = strsep(&tag_name, " ");

    char err_msg[strlen(MSG_NOT_IMPLEMENTED)
                 + strlen(tag_name)
                 + strlen(tag_ns)
                 + strlen(stanza_info->id)
                 + strlen(info->jid.local)
                 + strlen(info->jid.resource)
                 ];

    log_warn("Unimplemented stanza");

    snprintf(err_msg, sizeof(err_msg), MSG_NOT_IMPLEMENTED,
             tag_name, tag_ns, stanza_info->id, info->jid.local,
             info->jid.resource);
    check(sendall(info->fd, err_msg, strlen(err_msg)) > 0,
          "Error sending not supported error items");
    return;

error:
    XML_StopParser(info->parser, false);
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

    XML_SetEndElementHandler(info->parser, stanza_end);
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

    XML_SetEndElementHandler(info->parser, stanza_end);
    return;

error:
    /* TODO: Get rid of this error label.  We shouldn't stop the whole client
     * connection unless there is something really really wrong.  This also
     * will leak memory. */
    XML_StopParser(info->parser, false);
}
