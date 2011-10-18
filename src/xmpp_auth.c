/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implements the initial client authentication.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_auth.h"

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <expat.h>

#include "log.h"
#include "xmpp.h"

#define XMLNS_STREAM "http://etherx.jabber.org/streams"

static const char *XML_STREAM = XMLNS_STREAM " stream";

static const char *MSG_STREAM_HEADER =
    "<stream:stream "
        "from='localhost' "
        "id='foobarx' "
        "version='1.0' "
        "xml:lang='en' "
        "xmlns='jabber:client' "
        "xmlns:stream='http://etherx.jabber.org/streams'>";

static const char *MSG_STREAM_FEATURES_TLS =
    "<stream:features>"
        "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'>"
            "<required/>"
        "</starttls>"
    "</stream:features>";

static const char *MSG_STREAM_FEATURES_PLAIN =
    "<stream:features>"
        "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
            "<mechanism>PLAIN</mechanism>"
        "</mechanisms>"
    "</stream:features>";

void xmpp_auth_init_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Element start!");
    printf("<%s", name);

    for (int i = 0; attrs[i] != NULL; i += 2) {
        printf(" %s=\"%s\"", attrs[i], attrs[i + 1]);
    }
    printf(">\n");

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(name, XML_STREAM) == 0, "Unexpected message");

    // Send the stream header
    ssize_t numsent = 0;
    do {
        ssize_t newsent = send(info->fd, MSG_STREAM_HEADER + numsent,
                               strlen(MSG_STREAM_HEADER) - numsent, 0);
        check(newsent != -1, "Error sending stream header to client");
        numsent += newsent;
    } while (numsent < strlen(MSG_STREAM_HEADER));

    // Send the (plain) stream features
    numsent = 0;
    do {
        ssize_t newsent = send(info->fd,
                               MSG_STREAM_FEATURES_PLAIN + numsent,
                               strlen(MSG_STREAM_FEATURES_PLAIN) - numsent, 0);
        check(newsent != -1, "Error sending stream features to client");
        numsent += newsent;
    } while (numsent < strlen(MSG_STREAM_FEATURES_PLAIN));

    log_info("Sent stream features to client");

    return;

error:
    // Going to need to do something different here...
    XML_StopParser(info->parser, false);
}

void xmpp_auth_init_end(void *data, const char *name) {
    log_info("Element end!");
    printf("</%s>\n", name);
}

void xmpp_auth_init_data(void *data, const char *s, int len) {
    log_info("Element data!");
    printf("%.*s\n", len, s);
}
