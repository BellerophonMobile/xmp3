/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implements the initial client authentication.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

void xmpp_auth_init_start(void *data, const char *name, const char **attrs);
void xmpp_auth_init_end(void *data, const char *name);
void xmpp_auth_init_data(void *data, const char *s, int len);
