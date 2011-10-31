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

bool disco_query_info(struct xmpp_stanza *stanza, const char *name,
                      const char **attrs) {
    struct xmpp_client *client = stanza->from;

    log_info("Info Query IQ Start");

    XML_SetEndElementHandler(client->parser, query_info_end);
    return true;

    /*
error:
    xmpp_del_common_attrs(&stanza->attrs);
    free(stanza);
    XML_StopParser(client->parser, false);
    */
}

static void query_info_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from;
    xmpp_send_not_supported(stanza);
    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);
    return;
}

bool disco_query_items(struct xmpp_stanza *stanza, const char *name,
                       const char **attrs) {
    struct xmpp_client *client = stanza->from;

    log_info("Items Query IQ Start");

    XML_SetEndElementHandler(client->parser, query_items_end);
    return true;
}

static void query_items_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = stanza->from;
    xmpp_send_not_supported(stanza);
    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);
    return;
}
