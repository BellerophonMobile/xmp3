/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>

#include <netinet/in.h>

#include <expat.h>

#include "log.h"

struct jid {
    char *local;
    // domainpart is just "localhost"
    char *resource;
};

struct client_info {
    int fd;
    struct sockaddr_in caddr;
    XML_Parser parser;
    bool authenticated;
    struct jid jid;
};

/** Print out an XML start element and its attributes */
void xmpp_print_start_tag(const char *name, const char **attrs);

/** Print out an XML end element */
void xmpp_print_end_tag(const char *name);

/** Print out a string of XML data */
void xmpp_print_data(const char *s, int len);

/** Expat callback for when you do not expect a start tag. */
void xmpp_error_start(void *data, const char *name, const char **attrs);

/** Expat callback for when you do not expect an end tag. */
void xmpp_error_end(void *data, const char *name);

/** Expat callback for when you do not expect XML data. */
void xmpp_error_data(void *data, const char *s, int len);
