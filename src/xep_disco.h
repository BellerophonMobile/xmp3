/**
 * xmp3 - XMPP Proxy
 * xep_disco.{c,h} - Implements DISCO queries as specified in XEP-0030
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include "xmpp_common.h"
#include "xmpp_im.h"

void disco_query_info(struct iq_data *iq_data, const char *name,
                      const char **attrs);

void disco_query_items(struct iq_data *iq_data, const char *name,
                       const char **attrs);
