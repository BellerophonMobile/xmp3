/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implement initial XMPP authendication
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

struct xmpp_stanza;
struct xmpp_parser;

/**
 * Handler for the inital stream start to begin authentication.
 *
 * This should be the first handler defined on a new XML stream.
 *
 * @pre Assumes that this is the first handler called.  The client should send
 *      a starting <stream> tag.
 * @post On success, if not authenticated, proceeds to SASL authentication,
 *       else begins resource binding.  On failure (parse error, etc.), closes
 *       the connection and stops parsing.
 *
 * @param stanza The parsed stanza
 * @param parser The XML parser instance.
 * @param data   An xmpp_client instance.
 */
bool xmpp_auth_stream_start(struct xmpp_stanza *stanza,
                            struct xmpp_parser *parser, void *data);
