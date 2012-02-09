/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

const char *XMPP_IQ_SESSION_NS;

#if 0
const char *XMPP_IQ_QUERY_ROSTER;
const char *XMPP_IQ_DISCO_QUERY_INFO;
const char *XMPP_IQ_DISCO_QUERY_ITEMS;
#endif

// Forward declarations
struct xmpp_stanza;
struct xmp3_xml;

/** IQ stanza callback for handling a session IQ. */
bool xmpp_im_iq_session(struct xmpp_stanza *stanza, struct xmpp_server *server,
                        void *data);

#if 0
/** IQ stanza callback for handling a roster query. */
bool xmpp_im_iq_roster_query(struct xmpp_stanza *stanza, void *data);

bool xmpp_im_iq_disco_query_info(struct xmpp_stanza *stanza, void *data);

bool xmpp_im_iq_disco_query_items(struct xmpp_stanza *stanza, void *data);
#endif
