/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

#include "xmpp_common.h"

/** Expat handler for general XMPP stanza processing. */
void xmpp_core_stanza_start(void *data, const char *name, const char **attrs);

/** Expat handler for cleaning up from handling an XMPP stanza. */
void xmpp_core_stanza_end(void *data, const char *name);

/** Expat handler for handling the end of a stream. */
void xmpp_core_stream_end(void *data, const char *name);

/** Message stanza callback for handling a message stanza sent to a client. */
bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data);
