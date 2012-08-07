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
 * @file xmpp_im.h
 * Implements the parsing for RFC6121 IM and Presence
 */

#pragma once

#include <stdbool.h>

extern const char *XMPP_IQ_SESSION_NS;
extern const char *XMPP_IQ_DISCO_ITEMS_NS;
extern const char *XMPP_IQ_DISCO_INFO_NS;
extern const char *XMPP_IQ_ROSTER_NS;
extern const char *XMPP_IQ_PING_NS;
extern const char *XMPP_IQ_VCARD_TEMP_NS;

/* Forward declarations. */
struct xmpp_stanza;
struct xmp3_xml;

/** IQ stanza callback for handling a session IQ. */
bool xmpp_im_iq_session(struct xmpp_stanza *stanza, struct xmpp_server *server,
                        void *data);

/** IQ stanza callback for handling a disco items query. */
bool xmpp_im_iq_disco_items(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data);

/** IQ stanza callback for handling a disco info query. */
bool xmpp_im_iq_disco_info(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data);

/** IQ stanza callback for handing a roster query. */
bool xmpp_im_iq_roster(struct xmpp_stanza *stanza, struct xmpp_server *server,
                       void *data);

/** IQ stanza callback for handling a ping. */
bool xmpp_im_iq_ping(struct xmpp_stanza *stanza, struct xmpp_server *server,
                     void *data);

/** IQ stanza callback for handling a vcard request. */
bool xmpp_im_iq_vcard_temp(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data);
