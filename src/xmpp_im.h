/**
 * xmp3 - XMPP Proxy
 * xmpp_im.{c,h} - Implements the parsing for RFC6121 IM and Presence
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include "xmpp.h"

const char *XMPP_IQ_SESSION;
const char *XMPP_IQ_QUERY_ROSTER;
const char *XMPP_IQ_DISCO_QUERY_ITEMS;
const char *XMPP_IQ_DISCO_QUERY_INFO;

bool xmpp_im_iq_session(struct xmpp_stanza *stanza, void *data);

bool xmpp_im_iq_roster_query(struct xmpp_stanza *stanza, void *data);

/*
bool xmpp_im_iq_disco_query_items(struct xmpp_stanza *stanza, void *data);

bool xmpp_im_iq_disco_query_info(struct xmpp_stanza *stanza, void *data);
*/
