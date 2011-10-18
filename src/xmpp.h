/**
 * xmp3 - XMPP Proxy
 * xmpp.{c,h} - Implements the server part of a normal XMPP server.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

/* Forward declarations to cut down on includes. */
struct XML_ParserStruct;
typedef struct XML_ParserStruct *XML_Parser;

struct event_loop;

struct client_info {
    int fd;
    struct sockaddr_in caddr;
    XML_Parser parser;
};

/** Initializes the XMPP server and listens for new connections */
bool xmpp_init(struct event_loop *loop, struct in_addr addr, uint16_t port);
