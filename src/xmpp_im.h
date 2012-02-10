/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

extern const char *XMPP_IQ_SESSION_NS;
extern const char *XMPP_IQ_DISCO_ITEMS_NS;
extern const char *XMPP_IQ_DISCO_INFO_NS;
extern const char *XMPP_IQ_ROSTER_NS;

// Forward declarations
struct xmpp_stanza;
struct xmp3_xml;

/** IQ stanza callback for handling a session IQ. */
bool xmpp_im_iq_session(struct xmpp_stanza *stanza, struct xmpp_server *server,
                        void *data);

bool xmpp_im_iq_disco_items(struct xmpp_stanza *stanza,
                            struct xmpp_server *server, void *data);

bool xmpp_im_iq_disco_info(struct xmpp_stanza *stanza,
                           struct xmpp_server *server, void *data);

bool xmpp_im_iq_roster(struct xmpp_stanza *stanza, struct xmpp_server *server,
                       void *data);
