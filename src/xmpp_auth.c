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
#include "utils.h"

#include "xmpp_common.h"

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




void xmpp_auth_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stream_start, xmpp_error_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void stream_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Starting authentication...");
    print_start_tag(name, attrs);

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(name, XML_STREAM) == 0, "Unexpected message");

    // Send the stream header
    check(sendall(info->fd, MSG_STREAM_HEADER, strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");

        // Send the (plain) stream features
        check(sendall(info->fd, MSG_STREAM_FEATURES_PLAIN,
                      strlen(MSG_STREAM_FEATURES_PLAIN)) > 0,
              "Error sending plain stream features to client");
        XML_SetElementHandler(info->parser, auth_plain_start, auth_plain_end);
        XML_SetCharacterDataHandler(info->parser, auth_plain_data);

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

    XML_StopParser(info->parser, false);
}

    struct client_info *info = (struct client_info*)data;
    XML_StopParser(info->parser, false);
}

    XML_StopParser(info->parser, false);
}
