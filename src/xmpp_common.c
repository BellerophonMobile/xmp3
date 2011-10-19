/**
 * xmp3 - XMPP Proxy
 * xmpp_common.{c,h} - Common XMPP functions/data.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_common.h"

#include <stdio.h>

void xmpp_print_start_tag(const char *name, const char **attrs) {
    printf("\t<%s", name);

    for (int i = 0; attrs[i] != NULL; i += 2) {
        printf(" %s=\"%s\"", attrs[i], attrs[i + 1]);
    }
    printf(">\n");
}

void xmpp_print_end_tag(const char *name) {
    printf("\t</%s>\n", name);
}

void xmpp_print_data(const char *s, int len) {
    /* The %1$d means take the first argument (len) and format it as a decimal.
     * The %2$.*1$s:
     *   %2$ - Take the second argument (s)
     *   .*1$ - Use the first argument for the length (len)
     *   s    - Format it as a string (using len as length)
     */
    printf("\t%1$d bytes: '%2$.*1$s'\n", len, s);
}

void xmpp_error_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected start tag %s", name);
    xmpp_print_start_tag(name, attrs);
    XML_StopParser(info->parser, false);
}

void xmpp_error_end(void *data, const char *name) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected end tag %s", name);
    xmpp_print_end_tag(name);
    XML_StopParser(info->parser, false);
}

void xmpp_error_data(void *data, const char *s, int len) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected data");
    xmpp_print_data(s, len);
    XML_StopParser(info->parser, false);
}
