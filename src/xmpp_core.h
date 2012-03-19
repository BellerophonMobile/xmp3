/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

// Forward declarations
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
