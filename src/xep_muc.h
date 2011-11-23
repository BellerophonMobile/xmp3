/**
 * xmp3 - XMPP Proxy
 * xep_muc.{c,h} - Implements XEP-0045 Multi-User Chat
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

#include "xmpp_common.h"

extern const struct jid MUC_JID;

/** Opaque type for a MUC component. */
struct xep_muc;

/** Allocates and initializes a new MUC component. */
struct xep_muc* xep_muc_new();

/** Frees a MUC component. */
void xep_muc_del(struct xep_muc *muc);

/** Main stanza handler for group chat. */
bool xep_muc_stanza_handler(struct xmpp_stanza *stanza, void *data);
