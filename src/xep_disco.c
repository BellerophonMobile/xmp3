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

// Forward declarations
static void query_info_end(void *data, const char *name);
static void query_items_end(void *data, const char *name);

bool disco_query_info(struct stanza_info *stanza_info, const char *name,
                      const char **attrs) {
    struct client_info *client_info = stanza_info->client_info;

    log_info("Info Query IQ Start");

    XML_SetEndElementHandler(client_info->parser, query_info_end);
    return true;

    /*
error:
    xmpp_del_common_attrs(&stanza_info->attrs);
    free(stanza_info);
    XML_StopParser(client_info->parser, false);
    */
}

static void query_info_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *client_info = stanza_info->client_info;
    xmpp_send_not_supported(stanza_info);
    XML_SetEndElementHandler(client_info->parser, xmpp_im_stanza_end);
    return;
}

bool disco_query_items(struct stanza_info *stanza_info, const char *name,
                       const char **attrs) {
    struct client_info *client_info = stanza_info->client_info;

    log_info("Items Query IQ Start");

    XML_SetEndElementHandler(client_info->parser, query_items_end);
    return true;
}

static void query_items_end(void *data, const char *name) {
    struct stanza_info *stanza_info = (struct stanza_info*)data;
    struct client_info *client_info = stanza_info->client_info;
    xmpp_send_not_supported(stanza_info);
    XML_SetEndElementHandler(client_info->parser, xmpp_im_stanza_end);
    return;
}
