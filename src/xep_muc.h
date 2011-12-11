/**
 * xmp3 - XMPP Proxy
 * xep_muc.{c,h} - Implements XEP-0045 Multi-User Chat
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

// Forward declarations
struct xmpp_server;
struct xmpp_stanza;

/** Opaque type for a MUC component. */
struct xep_muc;

/** Allocates and initializes a new MUC component. */
struct xep_muc* xep_muc_new(struct xmpp_server *server);

/** Frees a MUC component. */
void xep_muc_del(struct xep_muc *muc);
