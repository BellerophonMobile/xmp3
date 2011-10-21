/**
 * xmp3 - XMPP Proxy
 * xmpp_im_iq.{c,h} - Implements IQ message handling for RFC6121
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include "xmpp_common.h"

void xmpp_im_iq_handle(struct client_info *info, const char **attrs);
