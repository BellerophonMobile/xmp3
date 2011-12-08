/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

// Forward declarations
struct xmpp_stanza;

/** Expat handler for general XMPP stanza processing. */
void xmpp_core_stanza_start(void *data, const char *name, const char **attrs);

/** Expat handler for cleaning up from handling an XMPP stanza. */
void xmpp_core_stanza_end(void *data, const char *name);

/** Expat handler for handling the end of a stream. */
void xmpp_core_stream_end(void *data, const char *name);

/** Callback for handling a stanza addressed to the server itself. */
bool xmpp_core_stanza_handler(struct xmpp_stanza *stanza, void *data);

/** Callback for handling a message stanza sent to a client. */
bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data);
