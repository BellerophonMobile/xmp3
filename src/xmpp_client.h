/*
 * Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file xmpp_client.h
 * A connected XMPP server client.
 */

#pragma once

/* Forward declarations. */
struct client_socket;
struct xmpp_client;
struct xmpp_parser;
struct xmpp_server;

struct xmpp_client* xmpp_client_new(struct xmpp_server *server,
                                    struct client_socket *socket);

void xmpp_client_del(struct xmpp_client *client);

/** Return the server this client is connected to. */
struct xmpp_server* xmpp_client_server(struct xmpp_client *client);

/** Return the socket used to communicate with this client. */
struct client_socket* xmpp_client_socket(struct xmpp_client *client);

/** Return the XMPP parser instance for this client. */
struct xmpp_parser* xmpp_client_parser(struct xmpp_client *client);

/** Return the JID for this client. */
struct jid* xmpp_client_jid(const struct xmpp_client *client);

/**
 * Client takes ownership of JID.
 */
void xmpp_client_set_jid(struct xmpp_client *client, struct jid *jid);
