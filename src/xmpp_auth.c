/**
 * xmp3 - XMPP Proxy
 * xmpp_auth.{c,h} - Implement initial XMPP authendication
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <utstring.h>

#include "log.h"
#include "utils.h"

#include "client_socket.h"
#include "jid.h"
#include "xmpp_client.h"
#include "xmpp_core.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"
#include "xmpp_parser.h"

#include "xmpp_auth.h"

// authzid, authcid, passed can be 255 octets, plus 2 NULLs inbetween
#define PLAIN_AUTH_BUFFER_SIZE (3 * 255 + 2)

// Maximum size of "id" attributes
#define ID_BUFFER_SIZE 256

// Maximum size of the "resourcepart" in resource binding
#define RESOURCEPART_BUFFER_SIZE 1024

// XML string constants
static const char *STREAM_NS = "http://etherx.jabber.org/streams";
static const char *STREAM_NAME = "stream";

static const char *STARTTLS_NS = "urn:ietf:params:xml:ns:xmpp-tls";
static const char *STARTTLS = "starttls";

static const char *SASL_NS = "urn:ietf:params:xml:ns:xmpp-sasl";
static const char *AUTH = "auth";
static const char *AUTH_MECHANISM = "mechanism";
static const char *AUTH_MECHANISM_PLAIN = "PLAIN";

static const char *BIND_NS = "urn:ietf:params:xml:ns:xmpp-bind";
static const char *BIND = "bind";
static const char *RESOURCE = "resource";

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
            "<jid>%s</jid>"
        "</bind>"
    "</iq>";

// Forward declarations
/* Inital authentication handlers
 * See: http://tools.ietf.org/html/rfc6120#section-9.1 */

static bool stream_sasl_start(struct xmpp_stanza *stanza,
                              struct xmpp_parser *parser, void *data);
static bool stream_bind_start(struct xmpp_stanza *stanza,
                              struct xmpp_parser *parser, void *data);

static bool handle_starttls(struct xmpp_stanza *stanza,
                            struct xmpp_parser *parser, void *data);

static bool handle_sasl_plain(struct xmpp_stanza *stanza,
                              struct xmpp_parser *parser, void *data);

static bool handle_bind_iq(struct xmpp_stanza *stanza,
                           struct xmpp_parser *parser, void *data);

/**
 * Step 1: Client initiates stream to server.
 *
 * Handles the starting <stream> tag.
 */
bool xmpp_auth_stream_start(struct xmpp_stanza *stanza,
                            struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("New stream start");

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(xmpp_stanza_namespace(stanza), STREAM_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), STREAM_NAME) == 0,
          "Unexpected stanza");

    // Step 2: Server responds by sending a response stream header to client
    check(client_socket_sendall(xmpp_client_socket(client),
                MSG_STREAM_HEADER, strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");

    /* Step 3: Server sends stream features to client (only the STARTTLS
     * extension at this point, which is mandatory-to-negotiate) */
    if (xmpp_server_ssl_context(xmpp_client_server(client)) != NULL) {
        check(client_socket_sendall(xmpp_client_socket(client),
                                    MSG_STREAM_FEATURES_TLS,
                                    strlen(MSG_STREAM_FEATURES_TLS)) > 0,
              "Error sending TLS stream features to client");

        // We expect to see a <starttls> tag from the client.
        xmpp_parser_set_handler(parser, handle_starttls);

    } else {
        // SSL is disabled, skip right to SASL.
        check(client_socket_sendall(xmpp_client_socket(client),
                    MSG_STREAM_FEATURES_SASL,
                    strlen(MSG_STREAM_FEATURES_SASL)) > 0,
              "Error sending SASL stream features to client");

        // We expect to see a SASL PLAIN <auth> tag next.
        xmpp_parser_set_handler(parser, handle_sasl_plain);
    }
    return true;

error:
    return false;
}

static bool stream_sasl_start(struct xmpp_stanza *stanza,
                              struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("SASL stream start");

    /* Step 7: If TLS negotiation is successful, client initiates a new stream
     * to server over the TLS-protected TCP connection. */
    check(strcmp(xmpp_stanza_namespace(stanza), STREAM_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), STREAM_NAME) == 0,
          "Unexpected stanza");

    /* Step 8: Server responds by sending a stream header to client along with
     * any available stream features. */
    check(client_socket_sendall(xmpp_client_socket(client),
                MSG_STREAM_HEADER, strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");
    check(client_socket_sendall(xmpp_client_socket(client),
                MSG_STREAM_FEATURES_SASL,
                strlen(MSG_STREAM_FEATURES_SASL)) > 0,
          "Error sending SASL stream features to client");

    // We expect to see a SASL PLAIN <auth> tag next.
    xmpp_parser_set_handler(parser, handle_sasl_plain);
    return true;

error:
    return false;
}

static bool stream_bind_start(struct xmpp_stanza *stanza,
                              struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Resource bind stream start");

    check(strcmp(xmpp_stanza_namespace(stanza), STREAM_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), STREAM_NAME) == 0,
          "Unexpected stanza");

    /* Step 14: Server responds by sending a stream header to client along
     * with supported features (in this case, resource binding) */
    check(client_socket_sendall(xmpp_client_socket(client),
                MSG_STREAM_HEADER, strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");
    check(client_socket_sendall(xmpp_client_socket(client),
                    MSG_STREAM_FEATURES_BIND,
                    strlen(MSG_STREAM_FEATURES_BIND)) > 0,
          "Error sending bind stream features to client");

    // We expect to see the resouce binding IQ stanza next.
    xmpp_parser_set_handler(parser, handle_bind_iq);
    return true;

error:
    return false;
}

/**
 * Begins TLS authentication.
 */
static bool handle_starttls(struct xmpp_stanza *stanza,
                            struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Start TLS");

    check(strcmp(xmpp_stanza_namespace(stanza), STARTTLS_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), STARTTLS) == 0,
          "Unexpected stanza");

    check(client_socket_sendall(xmpp_client_socket(client),
                MSG_TLS_PROCEED, strlen(MSG_TLS_PROCEED)) > 0,
          "Error sending TLS proceed to client");

    check(client_socket_ssl_new(
          xmpp_client_socket(client),
          xmpp_server_ssl_context(xmpp_client_server(client))) != NULL,
          "Error initializing SSL socket.");

    // We expect a new stream from the client
    xmpp_parser_new_stream(parser);
    xmpp_parser_set_handler(parser, stream_sasl_start);

    log_info("Done?");
    return true;

error:
    log_info("FAILED!");
    return false;
}

/**
 * Begins SASL PLAIN authentication.
 */
static bool handle_sasl_plain(struct xmpp_stanza *stanza,
                              struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("SASL plain authentication");

    check(strcmp(xmpp_stanza_namespace(stanza), SASL_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), AUTH) == 0,
          "Unexpected stanza");

    const char *mechanism = xmpp_stanza_attr(stanza, AUTH_MECHANISM);
    check(mechanism != NULL && strcmp(mechanism, AUTH_MECHANISM_PLAIN) == 0,
          "Unexpected authentication mechanism.");

    struct base64_decodestate state;
    base64_init_decodestate(&state);

    char plaintext[PLAIN_AUTH_BUFFER_SIZE];
    memset(plaintext, 0, PLAIN_AUTH_BUFFER_SIZE);

    check(xmpp_stanza_data_length(stanza) * 0.75 <= PLAIN_AUTH_BUFFER_SIZE,
          "Auth plaintext buffer overflow");

    base64_decode_block(xmpp_stanza_data(stanza),
                        xmpp_stanza_data_length(stanza), plaintext, &state);

    char *authzid = plaintext;
    char *authcid = memchr(authzid, '\0', PLAIN_AUTH_BUFFER_SIZE) + 1;
    char *passwd = memchr(authcid, '\0', PLAIN_AUTH_BUFFER_SIZE) + 1;

    debug("authzid = '%s', authcid = '%s', passwd = '%s'",
          authzid, authcid, passwd);

    // TODO: If we wanted to do any authentication, do it here.
    log_info("User authenticated");

    struct jid *jid = jid_new();
    check_mem(jid);

    jid_set_local(jid, authcid);
    jid_set_domain(jid,
            jid_domain(xmpp_server_jid(xmpp_client_server(client))));
    xmpp_client_set_jid(client, jid);

    // Success!
    check(client_socket_sendall(xmpp_client_socket(client),
                MSG_SASL_SUCCESS, strlen(MSG_SASL_SUCCESS)) > 0,
          "Error sending SASL success to client");

    // Go to step 7, the client needs to send us a new stream header.
    xmpp_parser_new_stream(parser);
    xmpp_parser_set_handler(parser, stream_bind_start);
    return true;

error:
    return false;
}

/**
 * Step 15: Client binds a resource
 */
static bool handle_bind_iq(struct xmpp_stanza *stanza,
                           struct xmpp_parser *parser, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Resource binding IQ");

    UT_string success_msg;
    utstring_init(&success_msg);

    check(strcmp(xmpp_stanza_namespace(stanza), XMPP_STANZA_NS_CLIENT) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_IQ) == 0,
          "Unexpected stanza");

    // Validate the correct attributes set on the start tag
    const char *type = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE);
    check(type != NULL && strcmp(type, XMPP_STANZA_TYPE_SET) == 0,
          "Unexpected bind iq type.");

    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    check(id != NULL, "Bind iq has no id.");

    // Jump to the inner <bind> tag
    stanza = xmpp_stanza_children(stanza);
    check(stanza != NULL, "Bind iq has no child.");
    check(strcmp(xmpp_stanza_namespace(stanza), BIND_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), BIND) == 0,
          "Unexpected stanza");

    // Jump to the inner <resource> tag
    stanza = xmpp_stanza_children(stanza);
    check(stanza != NULL, "Bind has no child.");
    check(strcmp(xmpp_stanza_namespace(stanza), BIND_NS) == 0,
          "Unexpected stanza");
    check(strcmp(xmpp_stanza_name(stanza), RESOURCE) == 0,
          "Unexpected stanza");

    // TODO: Check for duplicate resources

    // Copy the resource into the client information structure
    jid_set_resource(xmpp_client_jid(client), xmpp_stanza_data(stanza));

    char *jidstr = jid_to_str(xmpp_client_jid(client));
    utstring_printf(&success_msg, MSG_BIND_SUCCESS, id, jidstr);
    free(jidstr);

    /* Step 16: Server accepts submitted resourcepart and informs client of
     * successful resource binding */
    check(client_socket_sendall(xmpp_client_socket(client),
                                utstring_body(&success_msg),
                                utstring_len(&success_msg)) > 0,
          "Error sending resource binding success message.");
    utstring_done(&success_msg);

    /* Resource binding, and thus authentication, is complete!  Continue to
     * process general messages. */
    xmpp_parser_set_handler(parser, xmpp_core_handle_stanza);
    xmpp_server_add_stanza_route(xmpp_client_server(client),
                                 xmpp_client_jid(client),
                                 xmpp_core_route_client, client);
    return true;

error:
    utstring_done(&success_msg);
    return false;
}
