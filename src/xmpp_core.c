/**
 * xmp3 - XMPP Proxy
 * xmpp_core.{c,h} - Implement RFC6121 XMPP-CORE functions.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp_core.h"

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <expat.h>

#include "log.h"
#include "utils.h"

#include "xmpp_common.h"
#include "xmpp_im.h"

#define XMLNS_STREAM "http://etherx.jabber.org/streams"
#define XMLNS_SASL "urn:ietf:params:xml:ns:xmpp-sasl"
#define XMLNS_BIND "urn:ietf:params:xml:ns:xmpp-bind"
#define XMLNS_CLIENT "jabber:client"

// authzid, authcid, passed can be 255 octets, plus 2 NULLs inbetween
#define PLAIN_AUTH_BUFFER_SIZE 3 * 255 + 2

// Maximum size of "id" attributes
#define ID_BUFFER_SIZE 256

// Maximum size of the "resourcepart" in resource binding
#define RESOURCEPART_BUFFER_SIZE 1024

static const char *XML_STREAM = XMLNS_STREAM " stream";
static const char *XML_AUTH = XMLNS_SASL " auth";
static const char *XML_IQ = XMLNS_CLIENT " iq";
static const char *XML_BIND = XMLNS_BIND " bind";
static const char *XML_BIND_RESOURCE = XMLNS_BIND " resource";

static const char *XML_AUTH_MECHANISM = "mechanism";
static const char *XML_AUTH_MECHANISM_PLAIN = "PLAIN";

static const char *XML_IQ_ID = "id";
static const char *XML_IQ_TYPE = "type";
static const char *XML_IQ_TYPE_SET = "set";

/* TODO: Technically, the id field should be unique per stream on the server,
 * but it doesn't seem to reall matter. */
static const char *MSG_STREAM_HEADER =
    "<stream:stream "
        "from='localhost' "
        "id='foobarx' "
        "version='1.0' "
        "xml:lang='en' "
        "xmlns='jabber:client' "
        "xmlns:stream='http://etherx.jabber.org/streams'>";

#if 0
static const char *MSG_STREAM_FEATURES_TLS =
    "<stream:features>"
        "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'>"
            "<required/>"
        "</starttls>"
    "</stream:features>";
#endif

static const char *MSG_STREAM_FEATURES_PLAIN =
    "<stream:features>"
        "<mechanisms xmlns='urn:ietf:params:xml:ns:xmpp-sasl'>"
            "<mechanism>PLAIN</mechanism>"
        "</mechanisms>"
    "</stream:features>";

static const char *MSG_STREAM_FEATURES_BIND =
    "<stream:features>"
        "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'/>"
    "</stream:features>";

static const char *MSG_SASL_SUCCESS =
    "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>";

static const char *MSG_BIND_SUCCESS =
    "<iq id='%s' type='result'>"
        "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
            "<jid>%s@localhost/%s</jid>"
        "</bind>"
    "</iq>";

struct auth_plain_data {
    struct client_info *info;
    struct base64_decodestate state;
    int length;
    char plaintext[PLAIN_AUTH_BUFFER_SIZE];
};

struct resource_bind_data {
    struct client_info *info;
    char id[ID_BUFFER_SIZE];
    int length;
    char resource[RESOURCEPART_BUFFER_SIZE];
};

// Forward declarations
static void stream_start(void *data, const char *name, const char **attrs);

static void auth_plain_start(void *data, const char *name, const char **attrs);
static void auth_plain_data(void *data, const char *s, int len);
static void auth_plain_end(void *data, const char *name);

static void bind_iq_start(void *data, const char *name, const char **attrs);
static void bind_iq_end(void *data, const char *name);

static void bind_start(void *data, const char *name, const char **attrs);
static void bind_end(void *data, const char *name);

static void bind_resource_start(void *data, const char *name,
                                const char **attrs);
static void bind_resource_data(void *data, const char *s, int len);
static void bind_resource_end(void *data, const char *name);

void xmpp_core_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stream_start, xmpp_error_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void stream_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;

    log_info("Starting stream...");
    xmpp_print_start_tag(name, attrs);

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(name, XML_STREAM) == 0, "Unexpected message");

    // Send the stream header
    check(sendall(info->fd, MSG_STREAM_HEADER, strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");

    if (!info->authenticated) {
        // Send the (plain) stream features
        check(sendall(info->fd, MSG_STREAM_FEATURES_PLAIN,
                      strlen(MSG_STREAM_FEATURES_PLAIN)) > 0,
              "Error sending plain stream features to client");
        XML_SetElementHandler(info->parser, auth_plain_start, auth_plain_end);
        XML_SetCharacterDataHandler(info->parser, auth_plain_data);
    } else {
        // Send the resource binding feature
        check(sendall(info->fd, MSG_STREAM_FEATURES_BIND,
                      strlen(MSG_STREAM_FEATURES_BIND)) > 0,
              "Error sending bind stream features to client");
        XML_SetStartElementHandler(info->parser, bind_iq_start);
        XML_SetCharacterDataHandler(info->parser, xmpp_error_data);
    }

    log_info("Sent stream features to client");
    return;

error:
    XML_StopParser(info->parser, false);
}

static void auth_plain_start(void *data, const char *name, const char **attrs)
{
    struct client_info *info = (struct client_info*)data;
    log_info("Starting SASL plain...");
    xmpp_print_start_tag(name, attrs);

    check(strcmp(name, XML_AUTH) == 0, "Unexpected message");

    int i;
    for (i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XML_AUTH_MECHANISM) == 0) {
            check(strcmp(attrs[i + 1], XML_AUTH_MECHANISM_PLAIN) == 0,
                  "Unexpected authentication mechanism");
            break;
        }
    }
    check(attrs[i] != NULL, "Did not find 'mechanism=\"PLAIN\"' attribute");

    /* Use a temporary structure to maintain the state of the Base64 decode of
     * the username/password. */
    struct auth_plain_data *auth_data = calloc(1, sizeof(*auth_data));
    check_mem(auth_data);
    auth_data->info = info;
    base64_init_decodestate(&auth_data->state);
    XML_SetUserData(info->parser, auth_data);
    return;

error:
    XML_StopParser(info->parser, false);
}

static void auth_plain_data(void *data, const char *s, int len) {
    struct auth_plain_data *auth_data = (struct auth_plain_data*)data;
    struct client_info *info = auth_data->info;

    log_info("SASL data");
    xmpp_print_data(s, len);

    /* Expat explicitly does not guarantee that you will get all data in
     * one callback call.  Thus, we need to incrementally decode the data
     * as we get it. */

    check(auth_data->length + (len * 0.75) <= PLAIN_AUTH_BUFFER_SIZE,
          "Auth plaintext buffer overflow");

    auth_data->length += base64_decode_block(s, len,
            auth_data->plaintext + auth_data->length, &auth_data->state);
    return;

error:
    XML_SetUserData(info->parser, info);
    free(auth_data);
    XML_StopParser(info->parser, false);
}

static void auth_plain_end(void *data, const char *name) {
    struct auth_plain_data *auth_data = (struct auth_plain_data*)data;
    struct client_info *info = auth_data->info;

    log_info("SASL end tag");
    xmpp_print_end_tag(name);

    check(strcmp(name, XML_AUTH) == 0, "Unexpected message");

    char *authzid = auth_data->plaintext;
    char *authcid = memchr(authzid, '\0', PLAIN_AUTH_BUFFER_SIZE) + 1;
    char *passwd = memchr(authcid, '\0', PLAIN_AUTH_BUFFER_SIZE) + 1;

    debug("authzid = '%s', authcid = '%s', passwd = '%s'",
          authzid, authcid, passwd);

    // Copy the username into the client information structure
    info->jid.local = calloc(strlen(authcid) + 1, sizeof(char));
    check_mem(info->jid.local);
    strcpy(info->jid.local, authcid);

    // If we wanted to do any authentication, do it here.

    // Sucess!
    check(sendall(info->fd, MSG_SASL_SUCCESS, strlen(MSG_SASL_SUCCESS)) > 0,
          "Error sending SASL success to client");

    info->authenticated = true;

    // Client should send a new stream header to us, so reset handlers
    XML_SetElementHandler(info->parser, stream_start, xmpp_error_end);
    XML_SetCharacterDataHandler(info->parser, xmpp_error_data);

    // Clean up our temp auth_data structure.
    XML_SetUserData(info->parser, info);
    free(auth_data);

    log_info("User authenticated");

    return;

error:
    XML_SetUserData(info->parser, info);
    free(auth_data);
    XML_StopParser(info->parser, false);
}

static void bind_iq_start(void *data, const char *name, const char **attrs) {
    struct client_info *info = (struct client_info*)data;
    struct resource_bind_data *bind_data = NULL;

    log_info("Start resource binding IQ");
    xmpp_print_start_tag(name, attrs);

    check(strcmp(name, XML_IQ) == 0, "Unexpected message");

    int i;
    for (i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XML_IQ_TYPE) == 0) {
            check(strcmp(attrs[i + 1], XML_IQ_TYPE_SET) == 0,
                    "IQ type must be \"set\"");
            break;
        }
    }
    check(attrs[i] != NULL, "Did not find 'type=\"set\"' attribute");

    // Use a temporary structure to store the ID field of the IQ.
    bind_data = calloc(1, sizeof(*bind_data));
    check_mem(bind_data);
    bind_data->info = info;

    for (i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XML_IQ_ID) == 0) {
            strcpy(bind_data->id, attrs[i+1]);
            break;
        }
    }
    check(attrs[i] != NULL, "Did not find \"id\" attribute");

    XML_SetElementHandler(info->parser, bind_start, bind_end);
    XML_SetUserData(info->parser, bind_data);
    return;

error:
    free(bind_data);
    XML_StopParser(info->parser, false);
}

static void bind_iq_end(void *data, const char *name) {
    struct resource_bind_data *bind_data = (struct resource_bind_data*)data;
    struct client_info *info = bind_data->info;
    // Need space to construct our binding success message
    char success_msg[strlen(MSG_BIND_SUCCESS)
                     + strlen(bind_data->id)
                     + strlen(info->jid.local)
                     + strlen(info->jid.resource)];

    log_info("Bind IQ end");
    xmpp_print_end_tag(name);
    check(strcmp(name, XML_IQ) == 0, "Unexpected message");

    snprintf(success_msg, sizeof(success_msg), MSG_BIND_SUCCESS,
             bind_data->id, info->jid.local, info->jid.resource);
    check(sendall(info->fd, success_msg, strlen(success_msg)) > 0,
          "Error sending resource binding success message.");

    XML_SetUserData(info->parser, info);
    free(bind_data);

    /* Resource binding, and thus authentication, is complete!  Continue to
     * process general messages. */
    xmpp_im_set_handlers(info->parser);

    return;

error:
    XML_SetUserData(info->parser, info);
    free(bind_data);
    XML_StopParser(info->parser, false);
}

static void bind_start(void *data, const char *name, const char **attrs) {
    struct resource_bind_data *bind_data = (struct resource_bind_data*)data;
    struct client_info *info = bind_data->info;

    log_info("Start bind");
    xmpp_print_start_tag(name, attrs);

    check(strcmp(name, XML_BIND) == 0, "Unexpected message");
    XML_SetElementHandler(info->parser, bind_resource_start,
                          bind_resource_end);
    XML_SetCharacterDataHandler(info->parser, bind_resource_data);
    return;

error:
    XML_SetUserData(info->parser, info);
    free(bind_data);
    XML_StopParser(info->parser, false);
}

static void bind_end(void *data, const char *name) {
    struct resource_bind_data *bind_data = (struct resource_bind_data*)data;
    struct client_info *info = bind_data->info;

    check(strcmp(name, XML_BIND) == 0, "Unexpected message");
    XML_SetElementHandler(info->parser, xmpp_error_start, bind_iq_end);
    return;

error:
    XML_SetUserData(info->parser, info);
    free(bind_data);
    XML_StopParser(info->parser, false);
}

static void bind_resource_start(void *data, const char *name,
                                const char **attrs) {
    struct resource_bind_data *bind_data = (struct resource_bind_data*)data;
    struct client_info *info = bind_data->info;

    log_info("Start bind resource");
    xmpp_print_start_tag(name, attrs);

    check(strcmp(name, XML_BIND_RESOURCE) == 0, "Unexpected message");
    return;

error:
    XML_SetUserData(info->parser, info);
    free(bind_data);
    XML_StopParser(info->parser, false);
}

static void bind_resource_data(void *data, const char *s, int len) {
    struct resource_bind_data *bind_data = (struct resource_bind_data*)data;
    struct client_info *info = bind_data->info;

    check(bind_data->length + len < RESOURCEPART_BUFFER_SIZE,
          "Resource buffer overflow.");

    memcpy(bind_data->resource + bind_data->length, s, len);
    bind_data->length += len;
    return;

error:
    XML_SetUserData(info->parser, info);
    free(bind_data);
    XML_StopParser(info->parser, false);
}

static void bind_resource_end(void *data, const char *name) {
    struct resource_bind_data *bind_data = (struct resource_bind_data*)data;
    struct client_info *info = bind_data->info;

    check(strcmp(name, XML_BIND_RESOURCE) == 0, "Unexpected message");

    // Copy the resource into the client information structure
    info->jid.resource = calloc(strlen(bind_data->resource) + 1,
                                     sizeof(char));
    check_mem(info->jid.resource);
    strcpy(info->jid.resource, bind_data->resource);

    XML_SetElementHandler(info->parser, xmpp_error_start, bind_end);
    return;

error:
    XML_SetUserData(info->parser, info);
    free(bind_data);
    XML_StopParser(info->parser, false);
}
