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

// Forward declarations
static void stream_start(void *data, const char *name, const char **attrs);

static void auth_plain_start(void *data, const char *name, const char **attrs);
static void auth_plain_end(void *data, const char *name);
static void auth_plain_data(void *data, const char *s, int len);

static void error_start(void *data, const char *name, const char **attrs);
static void error_end(void *data, const char *name);
static void error_data(void *data, const char *s, int len);

void xmpp_auth_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stream_start, error_end);
    XML_SetCharacterDataHandler(parser, error_data);
}

static void print_start_tag(const char *name, const char **attrs) {
    printf("<%s", name);

    for (int i = 0; attrs[i] != NULL; i += 2) {
        printf(" %s=\"%s\"", attrs[i], attrs[i + 1]);
    }
    printf(">\n");
}

static void print_end_tag(const char *name) {
    printf("</%s>\n", name);
}

static void print_data(const char *s, int len) {
    printf("%d bytes: '%.*1$s'\n", len, s);
}

static void stream_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Starting authentication...");
    print_start_tag(name, attrs);

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

    XML_SetElementHandler(info->parser, auth_plain_start, auth_plain_end);
    XML_SetCharacterDataHandler(info->parser, auth_plain_data);

    return;

error:
    // Going to need to do something different here...
    XML_StopParser(info->parser, false);
}

static void auth_plain_start(void *data, const char *name, const char **attrs)
{
    log_info("Starting SASL plain...");
    print_start_tag(name, attrs);
}

static void auth_plain_end(void *data, const char *name) {
    log_info("SASL end tag");
    print_end_tag(name);
}

static void auth_plain_data(void *data, const char *s, int len) {
    log_info("SASL data");
    print_data(s, len);
}

static void error_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected start tag %s", name);
    print_start_tag(name, attrs);
    XML_StopParser(info->parser, false);
}

static void error_end(void *data, const char *name) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected end tag %s", name);
    print_end_tag(name);
    XML_StopParser(info->parser, false);
}

static void error_data(void *data, const char *s, int len) {
    struct client_info *info = (struct client_info*)data;
    log_err("Unexpected data");
    print_data(s, len);
    XML_StopParser(info->parser, false);
}
