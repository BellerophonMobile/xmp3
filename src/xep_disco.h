/**
 * xmp3 - XMPP Proxy
 * xep_disco.{c,h} - Implements DISCO queries as specified in XEP-0030
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include <stdbool.h>

#include "xmpp_common.h"

bool disco_query_items(struct stanza_info *stanza_info, const char *name,
                       const char **attrs);

bool disco_query_info(struct stanza_info *stanza_info, const char *name,
                      const char **attrs);
