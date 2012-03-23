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
 * @file xmpp_core.h
 * Handles base stanza routing
 */

#pragma once

#include <stdbool.h>

/* Forward declarations. */
struct xmpp_parser;
struct xmpp_server;
struct xmpp_stanza;

/**
 * An xmpp_parser handler for stanzas from a local client.
 *
 * @attr data An xmpp_client structure.
 */
bool xmpp_core_handle_stanza(struct xmpp_stanza *stanza,
                             struct xmpp_parser *parser, void *data);

/**
 * An xmpp_server stanza route for stanzas directed to a local client.
 *
 * @attr data An xmpp_client structure.
 */
bool xmpp_core_route_client(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data);

/**
 * An xmpp_server stanza route for stanzas directed to the server itself.
 *
 * @attr data A null pointer, we don't need it.
 */
bool xmpp_core_route_server(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data);
