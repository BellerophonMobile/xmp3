/**
 * xmp3 - XMPP Proxy
 * xmpp_client.{c,h} - A connected XMPP server client.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <expat.h>

struct xmpp_client;

// Forward declarations
struct xmpp_server;
struct client_socket;

struct xmpp_client* xmpp_client_new(struct xmpp_server *server,
                                    struct client_socket *socket);

void xmpp_client_del(struct xmpp_client *client);

struct xmpp_server* xmpp_client_server(struct xmpp_client *client);

struct client_socket* xmpp_client_socket(struct xmpp_client *client);

XML_Parser xmpp_client_parser(struct xmpp_client *client);

struct jid* xmpp_client_jid(const struct xmpp_client *client);

/**
 * Client takes ownership of JID.
 */
void xmpp_client_set_jid(struct xmpp_client *client, struct jid *jid);
