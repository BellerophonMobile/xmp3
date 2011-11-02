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

#include "xmpp.h"
#include "xmpp_common.h"
#include "xmpp_im.h"

/* This should probably go in the utils module, if I use it outside of here,
 * I will. */
#define ALLOC_COPY_STRING(a, b) do {
    a = calloc(strlen(b) + 1, sizeof(char));
    check_mem(a);
    strcpy(a, b);
} while (0)

#define ALLOC_PUSH_BACK(a, b) do {
    char *tmp = calloc(strlen(b), sizeof(*tmp));
    check_mem(tmp);
    utarray_push_back(a, tmp);
} while (0)

// authzid, authcid, passed can be 255 octets, plus 2 NULLs inbetween
#define PLAIN_AUTH_BUFFER_SIZE 3 * 255 + 2

// Maximum size of "id" attributes
#define ID_BUFFER_SIZE 256

// Maximum size of the "resourcepart" in resource binding
#define RESOURCEPART_BUFFER_SIZE 1024

// XML string constants
static const char *XMPP_AUTH_MECHANISM = "mechanism";
static const char *XMPP_AUTH_MECHANISM_PLAIN = "PLAIN";

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

// Temporary structures
struct auth_plain_tmp {
    struct xmpp_client *client;
    struct base64_decodestate state;
    int length;
    char plaintext[PLAIN_AUTH_BUFFER_SIZE];
};

struct resource_bind_tmp {
    struct xmpp_client *client;
    char id[ID_BUFFER_SIZE];
    int length;
    char resource[RESOURCEPART_BUFFER_SIZE];
};

// Forward declarations
static void stream_start(void *data, const char *name, const char **attrs);
static void stream_end(void *data, const char *name);

/* Inital authentication handlers */
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

/* General stream stanza handlers */
static void stanza_start(void *data, const char *name, const char **attrs);
static void stanza_end(void *data, const char *name);

/* Misc local functions */
static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                      const char *name, const char **attrs);
static void del_stanza(struct xmpp_stanza *stanza);
static void new_stanza_jid(struct jid *jid, const char *strjid);


void xmpp_auth_set_handlers(XML_Parser parser) {
    XML_SetElementHandler(parser, stream_start, xmpp_error_end);
    XML_SetCharacterDataHandler(parser, xmpp_error_data);
}

static void stream_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;

    log_info("Starting stream...");

    // name and attrs are guaranteed to be null terminated, so strcmp is OK.
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");

    // Send the stream header
    check(sendall(client->fd, MSG_STREAM_HEADER,
                  strlen(MSG_STREAM_HEADER)) > 0,
          "Error sending stream header to client");

    if (!client->authenticated) {
        // Send the (plain) stream features
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

static void stream_end(void *data, const char *name) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    check(strcmp(name, XMPP_STREAM) == 0, "Unexpected stanza");
    client->connected = false;
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
    XML_SetElementHandler(client->parser, stream_start, xmpp_error_end);
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
    XML_SetElementHandler(client->parser, stanza_start, stream_end);
    XML_SetUserData(client->parser, client);
    xmpp_message_register_route(server, &client->jid, xmpp_message_to_client,
                                client);

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

static void stanza_start(void *data, const char *name, const char **attrs) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    log_info("Stanza start");

    struct xmpp_stanza *stanza = new_stanza(client, name, attrs);
    XML_SetUserData(client->parser, stanza);
    XML_SetEndElementHandler(client->parser, xmpp_im_stanza_end);

    if (strcmp(name, XMPP_MESSAGE) == 0) {
        xmpp_message_route(stanza);
    } else if (strcmp(name, XMPP_PRESENCE) == 0) {
        handle_presence(stanza, attrs);
    } else if (strcmp(name, XMPP_IQ) == 0) {
        handle_iq(stanza, attrs);
    } else {
        log_warn("Unknown stanza");
    }
}

static void stanza_end(void *data, const char *name) {
    struct xmpp_stanza *stanza = (struct xmpp_stanza*)data;
    struct xmpp_client *client = (struct xmpp_client*)stanza->from;
    XML_SetUserData(client->parser, client);
    del_stanza(stanza);

    XML_SetElementHandler(client->parser, stanza_start, stream_end);
    XML_SetCharacterDataHandler(client->parser, xmpp_ignore_data);
}

static void new_stanza_jid(struct jid *jid, const char *strjid) {
    char *domainstr = strchr(strjid, '@');
    char *resourcestr = strchr(strjid, '/');

    int locallen;
    if (domainstr == NULL) {
        locallen = strlen(strjid);
    } else {
        locallen = domainstr - strjid;
    }

    jid->local = calloc(1, locallen);
    check_mem(jid->local);
    strncpy(jid->local, strjid, locallen);

    if (domainstr == NULL) {
        return;
    } else {
        domainstr++; // was previously on the '@' character in the string
    }

    int domainlen;
    if (resourcestr == NULL) {
        domainlen = strlen(domainstr);
    } else {
        domainlen = resourcestr - domainstr;
    }
    jid->domain = calloc(1, domainlen);
    check_mem(jid->domain);
    strncpy(jid->domain, domainstr, domainlen);

    if (resourcestr == NULL) {
        return;
    } else {
        resourcestr++; // was previously on the '/' character
    }

    jid->resource = calloc(1, strlen(resourcestr));
    check_mem(jid->resource);
    strncpy(jid->resource, resourcestr, strlen(resourcestr));
}

static struct xmpp_stanza* new_stanza(struct xmpp_client *client,
                                           const char *name,
                                           const char **attrs) {
    struct xmpp_stanza *stanza = calloc(1, sizeof(*stanza));
    check_mem(stanza);

    utarray_new(stanza->other_attrs, &ut_str_icd);

    ALLOC_COPY_STRING(stanza->name, name);

    /* RFC6120 Section 8.1.2.1 states that the server is to basically ignore
     * any "from" attribute in the stanza, and append the JID of the client the
     * server got the stanza from. */
    stanza->from = client;

    for (int i = 0; attrs[i] != NULL; i += 2) {
        if (strcmp(attrs[i], XMPP_ATTR_ID) == 0) {
            ALLOC_COPY_STRING(stanza->id, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TO) == 0) {
            new_stanza_jid(&stanza->to, attrs[i + 1]);
        } else if (strcmp(attrs[i], XMPP_ATTR_TYPE) == 0) {
            ALLOC_COPY_STRING(stanza->type, attrs[i + 1]);
        } else {
            ALLOC_PUSH_BACK(stanza->other_attrs, attrs[i]);
            ALLOC_PUSH_BACK(stanza->other_attrs, attrs[i + 1]);
        }
    }
    return stanza;
}

static void del_stanza(struct xmpp_stanza *stanza) {
    free(stanza->name);
    free(stanza->id);
    free(stanza->to.local);
    free(stanza->to.domain);
    free(stanza->to.resource);
    free(stanza->type);

    char **p = NULL;
    while ((p = utarray_next(stanza->other_attrs, p)) != NULL) {
        free(*p);
    }
    utarray_free(stanza->other_attrs);
    free(stanza);
}

