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

struct xep_namespaces {
    const char *namespace;
    xep_message_handler message_handler;
    xep_presence_handler presence_handler;
    xep_iq_handler iq_handler;
};

// XML String constants
static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

// Forward declarations
static void stanza_start(void *data, const char *name, const char **attrs);

// IQ Stanzas
static void handle_iq(struct client_info *info, const char **attrs);
static void iq_start(void *data, const char *name, const char **attrs);
static void iq_end(void *data, const char *name);

static void iq_session(struct iq_data *iq_data, const char *name,
                       const char **attrs);
static void iq_session_end(void *data, const char *name);

static void handle_presence(struct client_info *info, const char **attrs);
static void handle_message(struct client_info *info, const char **attrs);

static void message_error(struct message_data *iq_data, const char *name,
                          const char **attrs);
static void presence_error(struct presence_data *iq_data, const char *name,
                           const char **attrs);
static void iq_error(struct iq_data *iq_data, const char **attrs);

static struct xep_namespaces NAMESPACES[] = {
    {XMPP_NS_SESSION, message_error, presence_error, iq_session},
};

void xmpp_im_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stanza_start, xmpp_error_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void stanza_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Stanza start");
    xmpp_print_start_tag(name, attrs);

    if (strcmp(name, XMPP_IQ) == 0) {
        handle_iq(info, attrs);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        handle_presence(info, attrs);
    } else if (strcmp(name, XMPP_MESSAGE) == 0) {
        handle_message(info, attrs);
    } else {
        sentinel("Unexpected XMPP stanza type.");
    }
    return;

error:
    XML_StopParser(info->parser, false);
}

static void handle_iq(struct client_info *info, const char **attrs) {
    log_info("Got IQ msg");

    struct iq_data *iq_data = calloc(1, sizeof(*iq_data));
    check_mem(iq_data);
    iq_data->info = info;
    iq_data->end_handler = iq_end;

    xmpp_handle_common_attrs(&iq_data->attrs, attrs);

    XML_SetStartElementHandler(info->parser, iq_start);
    XML_SetUserData(info->parser, iq_data);
}

static void iq_start(void *data, const char *name, const char **attrs) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;

    log_info("IQ Start");
    xmpp_print_start_tag(name, attrs);

    int i;
    for (i = 0; i < sizeof(NAMESPACES) / sizeof(struct xep_namespaces); i++) {
        /* name is "namespace tag_name", a space inbetween.  We only want to
         * compare namespaces. */
        int ns_len = strchr(name, ' ') - name;
        if (strncmp(name, NAMESPACES[i].namespace, ns_len) == 0) {
            NAMESPACES[i].iq_handler(iq_data, name, attrs);
            break;
        }
    }

    if (i == sizeof(NAMESPACES)) {
        sentinel("Unexpected XMPP IQ type.");
    }
    return;

error:
    XML_SetUserData(info->parser, info);
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void iq_end(void *data, const char *name) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;

    log_info("IQ End");

    check(strcmp(name, XMPP_IQ) == 0, "Unexpected message");

    XML_SetUserData(info->parser, info);
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    xmpp_im_set_handlers(info->parser);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void iq_session(struct iq_data *iq_data, const char *name,
                                                const char **attrs) {
    struct client_info *info = iq_data->info;

    log_info("Session IQ Start");

    check(iq_data->attrs.id != NULL, "Session IQ needs id attribute");
    check(strcmp(iq_data->attrs.type, XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    XML_SetEndElementHandler(info->parser, iq_session_end);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void iq_session_end(void *data, const char *name) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;
    char stream_msg[strlen(MSG_STREAM_SUCCESS) + strlen(iq_data->attrs.id)];

    log_info("Session IQ End");
    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected IQ");

    snprintf(stream_msg, sizeof(stream_msg), MSG_STREAM_SUCCESS,
             iq_data->attrs.id);
    check(sendall(info->fd, stream_msg, strlen(stream_msg)) > 0,
          "Error sending stream success message.");

    XML_SetEndElementHandler(info->parser, iq_end);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void handle_presence(struct client_info *info, const char **attrs) {
    log_info("Got Presence msg");
}

static void handle_message(struct client_info *info, const char **attrs) {
    log_info("Got Message");
}

static void message_error(struct message_data *iq_data, const char *name,
                          const char **attrs) {
    log_err("Unexpected message stanza");
}

static void presence_error(struct presence_data *iq_data, const char *name,
                           const char **attrs) {
    log_err("Unexpected presence stanza");
}

static void iq_error(struct iq_data *iq_data, const char **attrs) {
    log_err("Unexpected iq stanza");
}
