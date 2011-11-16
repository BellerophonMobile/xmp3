/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implement initial XMPP authendication
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xmpp_auth.h"

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

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

#define XMPP_NS_TLS "urn:ietf:params:xml:ns:xmpp-tls"

// XML string constants
static const char *XMPP_STARTTLS = XMPP_NS_TLS XMPP_NS_SEPARATOR "starttls";
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
    "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'>"
        "<required/>"
    "</starttls>"
#endif

static const char *MSG_STREAM_FEATURES_TLS =
    "<stream:features>"
        "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'>"
            "<required/>"
        "</starttls>"
    "</stream:features>";

static const char *MSG_STREAM_FEATURES_SASL =
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

static const char *MSG_TLS_PROCEED =
    "<proceed xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>";

static const char *MSG_SASL_SUCCESS =
    "<success xmlns='urn:ietf:params:xml:ns:xmpp-sasl'/>";

static const char *MSG_BIND_SUCCESS =
    "<iq id='%s' type='result'>"
        "<bind xmlns='urn:ietf:params:xml:ns:xmpp-bind'>"
            "<jid>%s@localhost/%s</jid>"
        "</bind>"
    "</iq>";

/** Temporary structure used during SASL PLAIN authentication. */
struct auth_plain_tmp {
    /** Pointer back to the client this is for. */
    struct xmpp_client *client;

    /** Keeps track of the state of the base64 decoding. */
    struct base64_decodestate state;

    /** How far in our buffer we've written so far. */
    int length;

    /** Buffer to write our decoded base64 string to. */
    char plaintext[PLAIN_AUTH_BUFFER_SIZE];
};

/** Temporary structure used during resource binding. */
struct resource_bind_tmp {
    /** Pointer back to the client this is for. */
    struct xmpp_client *client;

    /** ID of the <iq> tag we are responding to. */
    char id[ID_BUFFER_SIZE];

    /** How far in our buffer we've written so far. */
    int length;

    /** Buffer to write our resource part. */
    char resource[RESOURCEPART_BUFFER_SIZE];
};

// Forward declarations
/* Inital authentication handlers
 * See: http://tools.ietf.org/html/rfc6120#section-9.1 */

static void stream_sasl_start(void *data, const char *name,
                              const char **attrs);
static void stream_bind_start(void *data, const char *name,
                              const char **attrs);

static void tls_start(void *data, const char *name, const char **attrs);
static void tls_end(void *data, const char *name);

static void sasl_plain_start(void *data, const char *name, const char **attrs);
static void sasl_plain_data(void *data, const char *s, int len);
static void sasl_plain_end(void *data, const char *name);

static void bind_iq_start(void *data, const char *name, const char **attrs);
static void bind_iq_end(void *data, const char *name);

static void bind_start(void *data, const char *name, const char **attrs);
static void bind_end(void *data, const char *name);

static void bind_resource_start(void *data, const char *name,
                                const char **attrs);
static void bind_resource_data(void *data, const char *s, int len);
static void bind_resource_end(void *data, const char *name);

/**
 * Step 1: Client initiates stream to server.
 *
 * Handles the starting <stream> tag.
 */
void xmpp_auth_stream_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Starting stream...");

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");

    // Step 2: Server responds by sending a response stream header to client
    check(sendall(client->socket, MSG_STREAM_HEADER,
                  strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");

    /* Step 3: Server sends stream features to client (only the STARTTLS
     * extension at this point, which is mandatory-to-negotiate) */
    check(sendall(client->socket, MSG_STREAM_FEATURES_TLS,
                  strlen(MSG_STREAM_FEATURES_TLS)) > 0,
          "Error sending TLS stream features to client");

    // We expect to see a <starttls> tag from the client.
    XML_SetElementHandler(client->parser, tls_start, tls_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);

    return;

error:
    XML_StopParser(client->parser, false);
}

static void stream_sasl_start(void *data, const char *name, const char **attrs)
{
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Starting SASL stream");

    /* Step 7: If TLS negotiation is successful, client initiates a new stream
     * to server over the TLS-protected TCP connection. */
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");

    /* Step 8: Server responds by sending a stream header to client along with
     * any available stream features. */
    check(sendall(client->socket, MSG_STREAM_HEADER,
                  strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");
    check(sendall(client->socket, MSG_STREAM_FEATURES_SASL,
                  strlen(MSG_STREAM_FEATURES_SASL)) > 0,
          "Error sending SASL stream features to client");

    // We expect to see a SASL PLAIN <auth> tag next.
    XML_SetElementHandler(client->parser, sasl_plain_start, sasl_plain_end);
    XML_SetCharacterDataHandler(client->parser, sasl_plain_data);
    return;

error:
    XML_StopParser(client->parser, false);
}

static void stream_bind_start(void *data, const char *name, const char **attrs)
{
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Starting resource bind stream");

    /* Step 14: Server responds by sending a stream header to client along
     * with supported features (in this case, resource binding) */
    check(sendall(client->socket, MSG_STREAM_HEADER,
                  strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");
    check(sendall(client->socket, MSG_STREAM_FEATURES_BIND,
                  strlen(MSG_STREAM_FEATURES_BIND)) > 0,
          "Error sending bind stream features to client");

    // We expect to see the resouce binding IQ stanza next.
    XML_SetStartElementHandler(client->parser, bind_iq_start);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);
    return;

error:
    XML_StopParser(client->parser, false);
}

/**
 * Handles the start <starttls> event.
 *
 * Begins TLS authentication.
 */
static void tls_start(void *data, const char *name, const char **attrs) {
    log_info("Starting TLS...");
}

/**
 * Handles the end <starttls> event.
 *
 * Confirms to the client that TLS is ok.
 */
static void tls_end(void *data, const char *name) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Initiating SSL connection...");

    check(strcmp(name, XMPP_STARTTLS) == 0, "Unexpected stanza");
    check(sendall(client->socket, MSG_TLS_PROCEED,
                  strlen(MSG_TLS_PROCEED)) > 0,
          "Error sending TLS proceed to client");

    xmpp_new_ssl_connection(client);

    // We expect a new stream from the client
    XML_SetElementHandler(client->parser, stream_sasl_start, xmpp_error_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_error_data);

    return;

error:
    XML_StopParser(client->parser, false);
}

/**
 * Begins SASL PLAIN authentication.
 *
 * Handles the starting <auth> tag.
 */
static void sasl_plain_start(void *data, const char *name, const char **attrs)
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
    XML_SetElementHandler(client->parser, xmpp_error_start, sasl_plain_end);
    return;

error:
    XML_StopParser(client->parser, false);
}

/**
 * Begins reading the base64 encoded username/password from the client.
 *
 * Handles the data between <auth> tags.
 */
static void sasl_plain_data(void *data, const char *s, int len) {
    struct auth_plain_tmp *auth_data = (struct auth_plain_tmp*)data;
    struct xmpp_client *client = auth_data->client;

    log_info("SASL data");

    /* Expat explicitly does not guarantee that you will get all data in
     * one callback call.  Thus, we need to incrementally decode the data
     * as we get it. */

    // length + (len * 0.75) is the maximum size of the base64 decoded string
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

/**
 * Finishes SASL PLAIN authentication.
 *
 * When we receive this, we've received the username/password from the client
 * and can authenticate.
 */
static void sasl_plain_end(void *data, const char *name) {
    struct auth_plain_tmp *auth_data = (struct auth_plain_tmp*)data;
    struct xmpp_client *client = auth_data->client;

    log_info("SASL end tag");

    check(strcmp(name, XMPP_AUTH) == 0, "Unexpected stanza");

    /* Now that we've received all the data between the auth tags, we can
     * determine the username and password. */
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

    // TODO: If we wanted to do any authentication, do it here.

    // Sucess!
    check(sendall(client->socket, MSG_SASL_SUCCESS,
                  strlen(MSG_SASL_SUCCESS)) > 0,
          "Error sending SASL success to client");

    client->authenticated = true;

    // Go to step 7, the client needs to send us a new stream header.
    XML_SetElementHandler(client->parser, stream_bind_start, xmpp_error_end);
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

/**
 * Step 15: Client binds a resource
 *
 * Handles the resource binding starting <iq> tag.
 */
static void bind_iq_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    struct resource_bind_tmp *bind_data = NULL;

    log_info("Start resource binding IQ");

    check(strcmp(name, XMPP_IQ) == 0, "Unexpected stanza");

    // Validate the correct attributes set on the start tag
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

    // Next, we expect to see the <bind> tag
    XML_SetElementHandler(client->parser, bind_start, bind_end);
    XML_SetUserData(client->parser, bind_data);
    return;

error:
    free(bind_data);
    XML_StopParser(client->parser, false);
}

/** Handles the resouce binding ending <iq> tag. */
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

    // Fill in our attributes to the success message
    snprintf(success_msg, sizeof(success_msg), MSG_BIND_SUCCESS,
             bind_data->id, client->jid.local, client->jid.resource);

    /* Step 16: Server accepts submitted resourcepart and informs client of
     * successful resource binding */
    check(sendall(client->socket, success_msg, strlen(success_msg)) > 0,
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

/** Handles the start <bind> tag inside a resource binding <iq>. */
static void bind_start(void *data, const char *name, const char **attrs) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    log_info("Start bind");

    check(strcmp(name, XMPP_BIND) == 0, "Unexpected stanza");

    // We expect to see a <resource> tag next
    XML_SetElementHandler(client->parser, bind_resource_start,
                          bind_resource_end);
    XML_SetCharacterDataHandler(client->parser, bind_resource_data);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

/** Handles the ending <bind> tag inside a resource binding <iq>. */
static void bind_end(void *data, const char *name) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    check(strcmp(name, XMPP_BIND) == 0, "Unexpected stanza");

    // We expect to see a closing </iq> tag next
    XML_SetElementHandler(client->parser, xmpp_error_start, bind_iq_end);
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

/** Handles the starting <resource> tag inside a resource binding <iq>. */
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

/** Handles the resouce string in the in a resource binding <iq>. */
static void bind_resource_data(void *data, const char *s, int len) {
    struct resource_bind_tmp *bind_data = (struct resource_bind_tmp*)data;
    struct xmpp_client *client = bind_data->client;

    check(bind_data->length + len < RESOURCEPART_BUFFER_SIZE,
          "Resource buffer overflow.");

    /* Since we might not get the whole resouce in one Expat callback call, we
     * keep track of how far we've written so far, and append to the end. */
    memcpy(bind_data->resource + bind_data->length, s, len);
    bind_data->length += len;
    return;

error:
    XML_SetUserData(client->parser, client);
    free(bind_data);
    XML_StopParser(client->parser, false);
}

/** Handles the ending <resource> tag inside a resource binding <iq>. */
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
