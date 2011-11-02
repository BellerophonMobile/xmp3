/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Handles base stanza routing
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>

#include "xmpp_common.h"

void xmpp_core_stanza_start(void *data, const char *name, const char **attrs);
void xmpp_core_stanza_end(void *data, const char *name);
void xmpp_core_stream_end(void *data, const char *name);

bool xmpp_core_message_handler(struct xmpp_stanza *from_stanza, void *data);
