/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implement initial XMPP authendication
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

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
 * @param data A struct xmpp_client for the newly connected client.
 * @param name The name of the starting tag that caused this event.
 * @param attrs The attributes of the starting tag that caused this event.
 */
void xmpp_auth_stream_start(void *data, const char *name, const char **attrs);
