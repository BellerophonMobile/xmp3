/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implements the initial client authentication.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_auth.h"

#include "log.h"

void xmpp_auth_init_start(void *data, const char *name, const char **attrs) {
    log_info("Element start!");
    printf("<%s", name);

    for (int i = 0; attrs[i] != NULL; i += 2) {
        printf(" %s=\"%s\"", attrs[i], attrs[i + 1]);
    }
    printf(">\n");
}

void xmpp_auth_init_end(void *data, const char *name) {
    log_info("Element end!");
    printf("</%s>\n", name);
}

void xmpp_auth_init_data(void *data, const char *s, int len) {
    log_info("Element data!");
    printf("%.*s\n", len, s);
}
