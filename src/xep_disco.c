/**
 * xmp3 - XMPP Proxy
 * xep_disco.{c,h} - Implements DISCO queries as specified in XEP-0030
 * Copyright (c) 2011 Drexel University
 */

#include "xep_disco.h"

#include <expat.h>

#include "log.h"
#include "utils.h"
#include "xmpp_common.h"
#include "xmpp_im.h"

// XML String constants
static const char *MSG_ITEMS_ERR =
    "<iq type='error' id='%s'>"
        "<query xmlns='http://jabber.org/protocol/disco#items'/>"
        "<error type='cancel'>"
            "<feature-not-implemented "
                "xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
        "</error>"
    "</iq>";

static const char *MSG_INFO_ERR =
    "<iq type='error' id='%s'>"
        "<query xmlns='http://jabber.org/protocol/disco#info'/>"
        "<error type='cancel'>"
            "<feature-not-implemented "
                "xmlns='urn:ietf:params:xml:ns:xmpp-stanzas'/>"
        "</error>"
    "</iq>";

// Forward declarations
static void query_info_end(void *data, const char *name);

static void query_items_end(void *data, const char *name);

void disco_query_info(struct iq_data *iq_data, const char *name,
                      const char **attrs) {
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
    char info_msg[strlen(MSG_INFO_ERR) + strlen(iq_data->attrs.id)];

    log_info("Info Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_INFO) == 0, "Unexpected IQ");

    snprintf(info_msg, sizeof(info_msg), MSG_ITEMS_ERR, iq_data->attrs.id);
    check(sendall(info->fd, info_msg, strlen(info_msg)) > 0,
          "Error sending Disco info query items");

    XML_SetEndElementHandler(info->parser, iq_data->end_handler);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}

void disco_query_items(struct iq_data *iq_data, const char *name,
                       const char **attrs) {
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
    char items_msg[strlen(MSG_ITEMS_ERR) + strlen(iq_data->attrs.id)];

    log_info("Items Query IQ End");
    check(strcmp(name, XMPP_IQ_QUERY_ITEMS) == 0, "Unexpected IQ");

    snprintf(items_msg, sizeof(items_msg), MSG_ITEMS_ERR, iq_data->attrs.id);
    check(sendall(info->fd, items_msg, strlen(items_msg)) > 0,
          "Error sending Disco item query items");

    XML_SetEndElementHandler(info->parser, iq_data->end_handler);
    return;

error:
    xmpp_del_common_attrs(&iq_data->attrs);
    free(iq_data);
    XML_StopParser(info->parser, false);
}
