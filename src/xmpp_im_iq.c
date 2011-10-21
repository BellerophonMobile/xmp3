/**
 * xmp3 - XMPP Proxy
 * xmpp_im_iq.{c,h} - Implements IQ message handling for RFC6121
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_im_iq.h"

#include <expat.h>

#include "log.h"
#include "utils.h"
#include "xmpp_common.h"
#include "xmpp_im.h"

// XML String constants
static const char *MSG_STREAM_SUCCESS =
    "<iq from='localhost' type='result' id='%s'/>";

// Temporary structures
struct iq_data {
    struct client_info *info;
    struct common_attrs attrs;
};

// Forward declarations
static void start(void *data, const char *name, const char **attrs);
static void end(void *data, const char *name);

static void handle_session(struct iq_data *iq_data, const char **attrs);
static void session_end(void *data, const char *name);

static void handle_query_info(struct iq_data *iq_data, const char **attrs);
static void query_info_end(void *data, const char *name);

static void handle_query_items(struct iq_data *iq_data, const char **attrs);
static void query_items_end(void *data, const char *name);

void xmpp_im_iq_handle(struct client_info *info, const char **attrs) {
    log_info("Got IQ msg");

    struct iq_data *iq_data = calloc(1, sizeof(*iq_data));
    check_mem(iq_data);
    iq_data->info = info;

    xmpp_handle_common_attrs(&iq_data->attrs, attrs);

    XML_SetStartElementHandler(info->parser, start);
    XML_SetUserData(info->parser, iq_data);
}

static void start(void *data, const char *name, const char **attrs) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;

    log_info("IQ Start");
    xmpp_print_start_tag(name, attrs);

    if (strcmp(name, XMPP_IQ_SESSION) == 0) {
        handle_session(iq_data, attrs);
    } else if (strcmp(name, XMPP_IQ_QUERY_INFO) == 0) {
        handle_query_info(iq_data, attrs);
    } else if (strcmp(name, XMPP_IQ_QUERY_ITEMS) == 0) {
        handle_query_items(iq_data, attrs);
    } else {
        sentinel("Unexpected XMPP IQ type.");
    }
    return;

error:
    XML_SetUserData(info->parser, info);
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void end(void *data, const char *name) {
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

static void handle_session(struct iq_data *iq_data, const char **attrs) {
    struct client_info *info = iq_data->info;

    log_info("Session IQ Start");

    check(iq_data->attrs.id != NULL, "Session IQ needs id attribute");
    check(strcmp(iq_data->attrs.type, XMPP_ATTR_TYPE_SET) == 0,
          "Session IQ type must be \"set\"");

    XML_SetEndElementHandler(info->parser, session_end);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void session_end(void *data, const char *name) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;
    char stream_msg[strlen(MSG_STREAM_SUCCESS) + strlen(iq_data->attrs.id)];

    log_info("Session IQ End");
    check(strcmp(name, XMPP_IQ_SESSION) == 0, "Unexpected IQ");

    snprintf(stream_msg, sizeof(stream_msg), MSG_STREAM_SUCCESS,
             iq_data->attrs.id);
    check(sendall(info->fd, stream_msg, strlen(stream_msg)) > 0,
          "Error sending stream success message.");

    XML_SetEndElementHandler(info->parser, end);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void handle_query_info(struct iq_data *iq_data, const char **attrs) {
    struct client_info *info = iq_data->info;

    log_info("Info Query IQ Start");

    XML_SetEndElementHandler(info->parser, query_info_end);
    return;

    /*
error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
    */
}

static void query_info_end(void *data, const char *name) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;

    log_info("Info Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_INFO) == 0, "Unexpected IQ");

    XML_SetEndElementHandler(info->parser, end);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

static void handle_query_items(struct iq_data *iq_data, const char **attrs) {
    struct client_info *info = iq_data->info;

    log_info("Items Query IQ Start");

    XML_SetEndElementHandler(info->parser, query_items_end);
    return;

    /*
error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
    */
}

static void query_items_end(void *data, const char *name) {
    struct iq_data *iq_data = (struct iq_data*)data;
    struct client_info *info = iq_data->info;

    log_info("Items Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_ITEMS) == 0, "Unexpected IQ");

    XML_SetEndElementHandler(info->parser, end);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}
