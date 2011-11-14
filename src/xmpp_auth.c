/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implement initial XMPP authendication
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
#include "xmpp.h"
#include "xmpp_core.h"

// authzid, authcid, passed can be 255 octets, plus 2 NULLs inbetween
#define PLAIN_AUTH_BUFFER_SIZE (3 * 255 + 2)

// Maximum size of "id" attributes
#define ID_BUFFER_SIZE 256

// Maximum size of the "resourcepart" in resource binding
#define RESOURCEPART_BUFFER_SIZE 1024

// XML string constants
static const char *XMPP_AUTH_MECHANISM = "mechanism";
static const char *XMPP_AUTH_MECHANISM_PLAIN = "PLAIN";

/* TODO: Technically, the id field should be unique per stream on the server,
 * but it doesn't seem to really matter. */
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
        "<session xmlns='urn:ietf:params:xml:ns:xmpp-session'/>"
    "</stream:features>";

static const char *MSG_SASL_SUCCESS =
    "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>";

static const char *MSG_BIND_SUCCESS =
    "<iq id='%s' type='result'>"
        "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
            "<jid>%s@localhost/%s</jid>"
        "</bind>"
    "</iq>";

// Temporary structure used during SASL PLAIN authentication
struct auth_plain_tmp {
    struct xmpp_client *client;
    struct base64_decodestate state;
    int length;
    char plaintext[PLAIN_AUTH_BUFFER_SIZE];
};

// Temporary structure used during resource binding
struct resource_bind_tmp {
    struct xmpp_client *client;
    char id[ID_BUFFER_SIZE];
    int length;
    char resource[RESOURCEPART_BUFFER_SIZE];
};

// Forward declarations
/* Inital authentication handlers
 * See: http://tools.ietf.org/html/rfc6120#section-9.1 */
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

/* Step 1: Client initiates stream to server. */
void xmpp_auth_stream_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Starting stream...");

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");

    // Step 2: Server responds by sending a response stream header to client
    check(sendall(client->fd, MSG_STREAM_HEADER,
                  strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");

    /* If we already authenticated, then we get ready for resource binding,
     * else we need to authenticate. */
    if (!client->authenticated) {
        /* Step 3: Server sends stream features to client (only the STARTTLS
         * extension at this point, which is mandatory-to-negotiate) */
        check(sendall(client->fd, MSG_STREAM_FEATURES_PLAIN,
                      strlen(MSG_STREAM_FEATURES_PLAIN)) > 0,
              "Error sending plain stream features to client");
        XML_SetElementHandler(client->parser, auth_plain_start,
                              auth_plain_end);
        XML_SetCharacterDataHandler(client->parser, auth_plain_data);
    } else {
        // Send the resource binding feature
        check(sendall(client->fd, MSG_STREAM_FEATURES_BIND,
                      strlen(MSG_STREAM_FEATURES_BIND)) > 0,
              "Error sending bind stream features to client");
        XML_SetStartElementHandler(client->parser, bind_iq_start);
        XML_SetCharacterDataHandler(client->parser, xmpp_error_data);
    }

    log_info("Sent stream features to client");
    return;

error:
    XML_StopParser(client->parser, false);
}

static void auth_plain_start(void *data, const char *name, const char **attrs)
{
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_info("Starting SASL plain...");

    check(strcmp(name, XMPP_AUTH) == 0, "Unexpected stanza");

    int i;
    for (i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_AUTH_MECHANISM) == 0) {
            check(strcmp(attrs[i + 1], XMPP_AUTH_MECHANISM_PLAIN) == 0,
                  "Unexpected authentication mechanism");
            break;
        }
    }
    check(attrs[i] != NULL, "Did not find 'mechanism=\"PLAIN\"' attribute");

    /* Use a temporary structure to maintain the state of the Base64 decode of
     * the username/password. */
    struct auth_plain_tmp *auth_data = calloc(1, sizeof(*auth_data));
    check_mem(auth_data);
    auth_data->client = client;
    base64_init_decodestate(&auth_data->state);
    XML_SetUserData(client->parser, auth_data);
    return;

error:
    XML_StopParser(client->parser, false);
}

static void auth_plain_data(void *data, const char *s, int len) {
    struct auth_plain_tmp *auth_data = (struct auth_plain_tmp*)data;
    struct xmpp_client *client = auth_data->client;

    log_info("SASL data");

    /* Expat explicitly does not guarantee that you will get all data in
     * one callback call.  Thus, we need to incrementally decode the data
     * as we get it. */

    check(auth_data->length + (len * 0.75) <= PLAIN_AUTH_BUFFER_SIZE,
          "Auth plaintext buffer overflow");

    auth_data->length += base64_decode_block(s, len,
            auth_data->plaintext + auth_data->length, &auth_data->state);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(auth_data);
    XML_StopParser(client->parser, false);
}

static void auth_plain_end(void *data, const char *name) {
    struct auth_plain_tmp *auth_data = (struct auth_plain_tmp*)data;
    struct xmpp_client *client = auth_data->client;

    log_info("SASL end tag");

    check(strcmp(name, XMPP_AUTH) == 0, "Unexpected stanza");

    char *authzid = auth_data->plaintext;
    char *authcid = memchr(authzid, '\0', PLAIN_AUTH_BUFFER_SIZE) + 1;
    char *passwd = memchr(authcid, '\0', PLAIN_AUTH_BUFFER_SIZE) + 1;

    debug("authzid = '%s', authcid = '%s', passwd = '%s'",
          authzid, authcid, passwd);

    // Copy the username into the client information structure
    client->jid.local = calloc(strlen(authcid) + 1, sizeof(char));
    check_mem(client->jid.local);
    strcpy(client->jid.local, authcid);

    client->jid.domain = calloc(strlen(SERVER_DOMAIN) + 1, sizeof(char));
    check_mem(client->jid.domain);
    strcpy(client->jid.domain, SERVER_DOMAIN);

    // If we wanted to do any authentication, do it here.

    // Sucess!
    check(sendall(client->fd, MSG_SASL_SUCCESS, strlen(MSG_SASL_SUCCESS)) > 0,
          "Error sending SASL success to client");

    client->authenticated = true;

    // Client should send a new stream header to us, so reset handlers
    XML_SetElementHandler(client->parser, xmpp_auth_stream_start,
                          xmpp_error_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);

    // Clean up our temp auth_data structure.
    XML_SetUserData(client->parser, client);
    free(auth_data);

    log_info("User authenticated");

    return;

error:
    XML_SetUserData(client->parser, client);
    free(auth_data);
    XML_StopParser(client->parser, false);
}

static void bind_iq_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    struct resource_bind_tmp *bind_data = NULL;

    log_info("Start resource binding IQ");

    check(strcmp(name, XMPP_IQ) == 0, "Unexpected stanza");

    int i;
    for (i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            check(strcmp(attrs[i + 1], XMPP_ATTR_TYPE_SET) == 0,
                    "IQ type must be \"set\"");
            break;
        }
    }
    check(attrs[i] != NULL, "Did not find 'type=\"set\"' attribute");

    // Use a temporary structure to store the ID field of the IQ.
    bind_data = calloc(1, sizeof(*bind_data));
    check_mem(bind_data);
    bind_data->client = client;

    for (i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_ID) == 0) {
            strcpy(bind_data->id, attrs[i+1]);
            break;
        }
    }
    check(attrs[i] != NULL, "Did not find \"id\" attribute");

    XML_SetElementHandler(client->parser, bind_start, bind_end);
    XML_SetUserData(client->parser, bind_data);
    return;

error:
    free(bind_data);
    XML_StopParser(client->parser, false);
}

static void bind_iq_end(void *data, const char *name) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;
    struct xmpp_server *server = client->server;
    // Need space to construct our binding success message
    char success_msg[strlen(MSG_BIND_SUCCESS)
                     + strlen(bind_data->id)
                     + strlen(client->jid.local)
                     + strlen(client->jid.resource)];

    log_info("Bind IQ end");
    check(strcmp(name, XMPP_IQ) == 0, "Unexpected stanza");

    snprintf(success_msg, sizeof(success_msg), MSG_BIND_SUCCESS,
             bind_data->id, client->jid.local, client->jid.resource);
    check(sendall(client->fd, success_msg, strlen(success_msg)) > 0,
          "Error sending resource binding success message.");


    /* Resource binding, and thus authentication, is complete!  Continue to
     * process general messages. */
    XML_SetElementHandler(client->parser, xmpp_core_stanza_start,
                          xmpp_core_stream_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);
    XML_SetUserData(client->parser, client);
    xmpp_register_message_route(server, &client->jid,
                                xmpp_core_message_handler, client);

    free(bind_data);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

static void bind_start(void *data, const char *name, const char **attrs) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    log_info("Start bind");

    check(strcmp(name, XMPP_BIND) == 0, "Unexpected stanza");
    XML_SetElementHandler(client->parser, bind_resource_start,
                          bind_resource_end);
    XML_SetCharacterDataHandler(client->parser, bind_resource_data);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

static void bind_end(void *data, const char *name) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    check(strcmp(name, XMPP_BIND) == 0, "Unexpected stanza");
    XML_SetElementHandler(client->parser, xmpp_error_start, bind_iq_end);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

static void bind_resource_start(void *data, const char *name,
                                const char **attrs) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    log_info("Start bind resource");

    check(strcmp(name, XMPP_BIND_RESOURCE) == 0, "Unexpected stanza");
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

static void bind_resource_data(void *data, const char *s, int len) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    check(bind_data->length + len < RESOURCEPART_BUFFER_SIZE,
          "Resource buffer overflow.");

    memcpy(bind_data->resource + bind_data->length, s, len);
    bind_data->length += len;
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

static void bind_resource_end(void *data, const char *name) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    check(strcmp(name, XMPP_BIND_RESOURCE) == 0, "Unexpected stanza");

    // Copy the resource into the client information structure
    client->jid.resource = calloc(strlen(bind_data->resource) + 1,
                                  sizeof(char));
    check_mem(client->jid.resource);
    strcpy(client->jid.resource, bind_data->resource);

    XML_SetElementHandler(client->parser, xmpp_error_start, bind_end);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}
