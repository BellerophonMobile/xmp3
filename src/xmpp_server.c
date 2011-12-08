/**
 * xmp3 - XMPP Proxy
 * xmpp_server.{c,h} - Main XMPP server data/functions
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "uthash.h"
#include "utlist.h"

#include "log.h"

#include "client_socket.h"
#include "event.h"
#include "jid.h"
#include "xep_muc.h"
#include "xmp3_options.h"
#include "xmpp_client.h"
#include "xmpp_core.h"
#include "xmpp_common.h"
#include "xmpp_im.h"
#include "xmpp_stanza.h"

#include "xmpp_server.h"

/**
 * Generic shortcut to add a callback to one of the server's lists.
 *
 * @param type The type of the list (no struct keyword).
 * @param list A pointer to the head of the list to manipulate.
 * @param ...  The arguments to the corresponding type's new function.
 */
#define ADD_CALLBACK(type, list, ...) do { \
    struct type *add = type ## _new(__VA_ARGS__); \
    check_mem(add); \
    struct type *match = NULL; \
    DL_SEARCH(list, match, add, type ## _cmp); \
    if (match != NULL) { \
        log_warn("Attempted to add duplicate callback."); \
        free(add); \
    } else { \
        DL_APPEND(list, add); \
    } \
} while (0)

/**
 * Generic shortcut to remove a callback to one of the server's lists.
 *
 * @param type The type of the list (no struct keyword).
 * @param list A pointer to the head of the list to manipulate.
 * @param ...  The arguments to the corresponding type's new function.
 */
#define DEL_CALLBACK(type, list, ...) do {\
    struct type *search = type ## _new(__VA_ARGS__); \
    struct type *match = NULL; \
    DL_SEARCH(list, match, search, type ## _cmp); \
    if (match == NULL) { \
        log_warn("Attempted to remove non-existent callback."); \
    } else { \
        DL_DELETE(list, match); \
        type ## _del(match); \
    } \
} while (0)

/**
 * Convenience macro to delete one of the server's callback lists.
 *
 * @param type The type of the list (no struct keyword).
 * @param list A pointer to the head of the list to manipulate.
 */
#define DELETE_LIST(type, list) do {\
    struct type *a, *a_tmp; \
    DL_FOREACH_SAFE(list, a, a_tmp) { \
        DL_DELETE(list, a); \
        type ## _del(a); \
    } \
} while (0)

struct c_client {
    struct xmpp_client *client;

    /** @{ Clients are stored in a doubly-linked list. */
    struct c_client *prev;
    struct c_client *next;
    /** @} */
};

/** Holds data on how to send a stanza to a particular JID. */
struct stanza_route {
    /** The JID to send to. */
    struct jid *jid;

    /** The function that will deliver the stanza. */
    xmpp_server_stanza_callback cb;

    /** Arbitrary data. */
    void *data;

    /** @{ These are kept in a doubly-linked list. */
    struct stanza_route *prev;
    struct stanza_route *next;
    /** @} */
};

/**
 * Holds data on how to handle a particular iq stanza.
 *
 * This is determined by the namespace + tag name of the first child element
 * (the spec says there can only be one child).
 */
struct iq_route {
    /** The namespace + name of the tag to match. */
    char *ns;

    /** The function that will deliver the stanza. */
    xmpp_server_stanza_callback cb;

    /** Arbitrary data that callbacks can use. */
    void *data;

    /** @{ These are kept in a doubly-linked list. */
    struct iq_route *prev;
    struct iq_route *next;
    /** @} */
};

/** Holds data on how to notify a component when a client disconnects. */
struct client_listener {
    /** The client to watch for disconnect events for. */
    struct xmpp_client *client;

    /** Function that will be called when the client disconnects. */
    xmpp_server_client_callback cb;

    /** Arbitrary data that callbacks can use. */
    void *data;

    /** @{ These are kept in a doubly-linked list. */
    struct client_listener *prev;
    struct client_listener *next;
    /** @} */
};

/** Holds data on a XMPP server (connected clients, routes, etc.). */
struct xmpp_server {
    /** The bound and listening file descriptor. */
    int fd;

    /** Buffer used to store incoming data. */
    char *buffer;

    /** The size of the server's receive buffer. */
    size_t buffer_size;

    int backlog;

    /** The event loop this server is registered on. */
    struct event_loop *loop;

    /** OpenSSL context. */
    SSL_CTX *ssl_context;

    /** The JID of this server. */
    struct jid *jid;

    /** Linked list of connected clients. */
    struct c_client *clients;

    /** Linked list of stanza routes. */
    struct stanza_route *stanza_routes;

    /** Linked list of iq routes. */
    struct iq_route *iq_routes;

    /** Linked list of client disconnect callbacks. */
    struct client_listener *client_listeners;

    /* TODO: There should be a more generic interface for components to
     * register with the server, probably a delegate model like in ontonet. */

    /** The MUC component. */
    struct xep_muc *muc;
};

// Forward declarations
static bool init_socket(struct xmpp_server *server,
                        const struct xmp3_options *options);
static bool init_ssl(struct xmpp_server *server,
                     const struct xmp3_options *options);
static bool init_components(struct xmpp_server *server,
                            const struct xmp3_options *options);

static void connect_client(struct event_loop *loop, int fd, void *data);
static void read_client(struct event_loop *loop, int fd, void *data);

static struct client_listener* client_listener_new(struct xmpp_client *client,
        xmpp_server_client_callback cb, void *data);
static void client_listener_del(struct client_listener *listener);
static int client_listener_cmp(const struct client_listener *a,
                               const struct client_listener *b);

static struct stanza_route* stanza_route_new(const struct jid *jid,
        xmpp_server_stanza_callback cb, void *data);
static void stanza_route_del(struct stanza_route *route);
static int stanza_route_cmp(const struct stanza_route *a,
                            const struct stanza_route *b);

static struct iq_route* iq_route_new(const char *ns,
        xmpp_server_stanza_callback cb, void *data);
static void iq_route_del(struct iq_route *route);
static int iq_route_cmp(const struct iq_route *a, const struct iq_route *b);

struct xmpp_server* xmpp_server_new(struct event_loop *loop,
                                    const struct xmp3_options *options) {
    struct xmpp_server *server = calloc(1, sizeof(*server));
    check_mem(server);

    server->buffer_size = xmp3_options_get_buffer_size(options);
    server->buffer = calloc(server->buffer_size, sizeof(*server->buffer));
    check_mem(server->buffer);

    server->backlog = xmp3_options_get_backlog(options);

    server->loop = loop;
    server->jid = jid_new_from_str(xmp3_options_get_server_name(options));
    check_mem(server->jid);

    if (xmp3_options_get_ssl(options)) {
        check(init_ssl(server, options), "Unable to initialize OpenSSL.");
    }

    check(init_socket(server, options), "Unable to initialize socket.");
    check(init_components(server, options),
          "Unable to initialize components.");

    // Register the event handler so we can get notified of new connections.
    event_register_callback(loop, server->fd, connect_client, server);

    log_info("Listening for XMPP connections on %s:%d",
             inet_ntoa(xmp3_options_get_addr(options)),
             xmp3_options_get_port(options));

    return server;

error:
    xmpp_server_del(server);
    return NULL;
}

void xmpp_server_del(struct xmpp_server *server) {
    xep_muc_del(server->muc);

    DELETE_LIST(stanza_route, server->stanza_routes);
    DELETE_LIST(iq_route, server->iq_routes);
    DELETE_LIST(client_listener, server->client_listeners);

    struct c_client *connected_client = NULL;
    struct c_client *connected_client_tmp = NULL;
    DL_FOREACH_SAFE(server->clients, connected_client, connected_client_tmp) {
        DL_DELETE(server->clients, connected_client);
        xmpp_client_del(connected_client->client);
        free(connected_client);
    }

    if (server->fd != -1) {
        close(server->fd);
    }
    if (server->buffer) {
        free(server->buffer);
    }
    if (server->ssl_context) {
        SSL_CTX_free(server->ssl_context);
    }
    free(server);
}

struct event_loop* xmpp_server_loop(const struct xmpp_server *server) {
    return server->loop;
}

const struct jid* xmpp_server_jid(const struct xmpp_server *server) {
    return server->jid;
}

SSL_CTX* xmpp_server_ssl_context(const struct xmpp_server *server) {
    return server->ssl_context;
}

void xmpp_server_add_client_listener(struct xmpp_client *client,
                                     xmpp_server_client_callback cb,
                                     void *data) {
    struct xmpp_server *server = xmpp_client_server(client);
    ADD_CALLBACK(client_listener, server->client_listeners, client, cb, data);
}

void xmpp_server_del_client_disconnect_cb(struct xmpp_client *client,
                                          xmpp_server_client_callback cb,
                                          void *data) {
    struct xmpp_server *server = xmpp_client_server(client);
    DEL_CALLBACK(client_listener, server->client_listeners, client, cb, data);
}

void xmpp_server_disconnect_client(struct xmpp_client *client) {
    struct xmpp_server *server = xmpp_client_server(client);

    // Sanity check that the client is registered with the server
    struct c_client *search = NULL;
    DL_FOREACH(server->clients, search) {
        if (client == search->client) {
            break;
        }
    }
    if (search == NULL) {
        log_warn("Attempted to disconnect non-registered client.");
        return;
    }

    struct client_listener *listener = NULL;
    struct client_listener *tmp = NULL;
    DL_FOREACH_SAFE(server->client_listeners, listener, tmp) {
        if (listener->client == client) {
            listener->cb(client, listener->data);
        }
        DL_DELETE(server->client_listeners, listener);
        free(listener);
    }

    DL_DELETE(server->clients, search);
    xmpp_client_del(client);
    free(search);
}

void xmpp_server_add_stanza_route(struct xmpp_server *server,
                                  const struct jid *jid,
                                  xmpp_server_stanza_callback cb, void *data) {
    ADD_CALLBACK(stanza_route, server->stanza_routes, jid, cb, data);
}

void xmpp_server_del_stanza_route(struct xmpp_server *server,
                                  const struct jid *jid,
                                  xmpp_server_stanza_callback cb, void *data) {
    DEL_CALLBACK(stanza_route, server->stanza_routes, jid, cb, data);
}

bool xmpp_server_route_stanza(struct xmpp_stanza *stanza) {
    struct xmpp_client *client = xmpp_stanza_client(stanza);
    struct xmpp_server *server = xmpp_client_server(client);
    const struct jid *search_jid = xmpp_stanza_jid_from(stanza);

    bool was_handled = false;

    struct stanza_route *route = NULL;
    DL_FOREACH(server->stanza_routes, route) {
        if (jid_cmp_wildcards(search_jid, route->jid) == 0) {
            was_handled = route->cb(stanza, route->data);
        }
    }
    if (!was_handled) {
        log_info("No route for destination");
    }
    return was_handled;
}

void xmpp_server_add_iq_route(struct xmpp_server *server, const char *ns,
                              xmpp_server_stanza_callback cb, void *data) {
    ADD_CALLBACK(iq_route, server->iq_routes, ns, cb, data);
}

void xmpp_server_del_iq_route(struct xmpp_server *server, const char *ns,
                              xmpp_server_stanza_callback cb, void *data) {
    DEL_CALLBACK(iq_route, server->iq_routes, ns, cb, data);
}

bool xmpp_server_route_iq(struct xmpp_stanza *stanza) {
    struct xmpp_client *client = xmpp_stanza_client(stanza);
    struct xmpp_server *server = xmpp_client_server(client);

    bool was_handled = false;

    struct iq_route *route = NULL;
    DL_FOREACH(server->iq_routes, route) {
        if (strcmp(xmpp_stanza_ns_name(stanza), route->ns) == 0) {
            was_handled = route->cb(stanza, route->data);
        }
    }
    if (!was_handled) {
        log_info("No route for destination");
    }
    return was_handled;
}

static bool init_socket(struct xmpp_server *server,
                        const struct xmp3_options *options) {
    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    check(server->fd != -1, "Error creating XMPP server socket");

    // Allow address reuse when in the TIME_WAIT state.
    static const int on = 1;
    check(setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &on,
                     sizeof(on)) != -1,
          "Error setting SO_REUSEADDR on server socket");

    struct sockaddr_in saddr = {
        .sin_family = AF_INET,
        .sin_port = htons(xmp3_options_get_port(options)),
        .sin_addr = xmp3_options_get_addr(options),
    };

    check(bind(server->fd, (struct sockaddr*)&saddr, sizeof(saddr)) != -1,
          "XMPP server socket bind error");
    check(listen(server->fd, server->backlog) != -1,
          "XMPP server socket listen error");

    return true;
error:
    return false;
}

static bool init_ssl(struct xmpp_server *server,
                     const struct xmp3_options *options) {
    server->ssl_context = SSL_CTX_new(SSLv23_server_method());
    check(server->ssl_context != NULL, "Cannot create SSL context.");
    check(SSL_CTX_use_certificate_chain_file(server->ssl_context,
                xmp3_options_get_certificate(options)) == 1,
            "Cannot load SSL certificate.");
    check(SSL_CTX_use_PrivateKey_file(server->ssl_context,
                xmp3_options_get_keyfile(options), SSL_FILETYPE_PEM) == 1,
            "Cannot load SSL private key.");
    check(SSL_CTX_check_private_key(server->ssl_context) == 1,
            "Invalid certificate/private key combination.");
    return true;

error:
    ERR_print_errors_fp(stderr);
    if (server->ssl_context) {
        SSL_CTX_free(server->ssl_context);
        server->ssl_context = NULL;
    }
    return false;
}

static bool init_components(struct xmpp_server *server,
                            const struct xmp3_options *options) {
    xmpp_server_add_stanza_route(server, server->jid,
                                 xmpp_core_stanza_handler, NULL);

    server->muc = xep_muc_new(server);
    check_mem(server->muc);

    xmpp_server_add_iq_route(server, XMPP_IQ_SESSION,
                             xmpp_im_iq_session, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_QUERY_ROSTER,
                             xmpp_im_iq_roster_query, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_DISCO_QUERY_INFO,
                             xmpp_im_iq_disco_query_info, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_DISCO_QUERY_ITEMS,
                             xmpp_im_iq_disco_query_items, NULL);
    return true;
}

static void connect_client(struct event_loop *loop, int fd, void *data) {
    struct xmpp_server *server = (struct xmpp_server*)data;

    struct sockaddr_in caddr;
    socklen_t caddrlen = sizeof(caddr);

    int client_fd = accept(fd, (struct sockaddr*)&caddr, &caddrlen);
    check(client_fd != -1, "Error accepting new client connection");

    struct client_socket *socket = client_socket_new(client_fd, caddr);
    check_mem(socket);

    struct xmpp_client *client = xmpp_client_new(server, socket);
    check_mem(client);

    event_register_callback(loop, client_fd, read_client, client);

    struct c_client *connected_client = calloc(1, sizeof(*connected_client));
    connected_client->client = client;

    log_info("New connection from %s:%d", inet_ntoa(caddr.sin_addr),
             caddr.sin_port);

    DL_APPEND(server->clients, connected_client);
    return;

error:
    if (connected_client) {
        free(connected_client);
    }
    if (client) {
        xmpp_client_del(client);
    }
    if (socket) {
        client_socket_del(socket);
    }
    if (client_fd != -1) {
        close(client_fd);
    }
}

static void read_client(struct event_loop *loop, int fd, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    struct xmpp_server *server = xmpp_client_server(client);

    ssize_t numrecv = client_socket_recv(xmpp_client_socket(client),
                                         server->buffer, server->buffer_size);

    if (numrecv == 0 || numrecv == -1) {
        char *addrstr = client_socket_addr_str(
                xmpp_client_socket(client));
        check_mem(addrstr);
        switch (numrecv) {
            case 0:
                log_info("%s disconnected", addrstr);
                break;
            case -1:
                log_err("Error reading from %s %s", addrstr, strerror(errno));
                break;
        }
        free(addrstr);
        goto error;
    }

    char *addrstr = client_socket_addr_str(xmpp_client_socket(client));
    check_mem(addrstr);
    log_info("%s - Read %zd bytes", addrstr, numrecv);
    free(addrstr);
    xmpp_print_data(server->buffer, numrecv);

    enum XML_Status status = XML_Parse(
            xmpp_client_parser(client), server->buffer, numrecv, false);
    check(status != XML_STATUS_ERROR, "Error parsing XML: %s",
          XML_ErrorString(XML_GetErrorCode(xmpp_client_parser(client))));

    return;

error:
    xmpp_server_disconnect_client(client);
}

static struct client_listener* client_listener_new(struct xmpp_client *client,
        xmpp_server_client_callback cb, void *data) {
    struct client_listener *listener = calloc(1, sizeof(*listener));
    check_mem(listener);

    listener->client = client;
    listener->cb = cb;
    listener->data = data;

    return listener;
}

static void client_listener_del(struct client_listener *listener) {
    free(listener);
}

static int client_listener_cmp(const struct client_listener *a,
                               const struct client_listener *b) {
    if (a->client != b->client) {
        return a - b;
    }
    if (a->cb != b->cb) {
        return a->cb - b->cb;
    }
    if (a->data != b->data) {
        return a->data - b->data;
    }
    return 0;
}

static struct stanza_route* stanza_route_new(const struct jid *jid,
        xmpp_server_stanza_callback cb, void *data) {
    struct stanza_route *route = calloc(1, sizeof(*route));
    check_mem(route);

    route->jid = jid_new_from_jid(jid);
    check_mem(route->jid);
    route->cb = cb;
    route->data = data;

    return route;
}

static void stanza_route_del(struct stanza_route *route) {
    jid_del(route->jid);
    free(route);
}

static int stanza_route_cmp(const struct stanza_route *a,
                            const struct stanza_route *b) {
    int rv = jid_cmp(a->jid, b->jid);
    if (rv != 0) {
        return rv;
    }
    if (a->cb != b->cb) {
        return a->cb - b->cb;
    }
    if (a->data != b->data) {
        return a->data - b->data;
    }
    return 0;
}

static struct iq_route* iq_route_new(const char *ns,
        xmpp_server_stanza_callback cb, void *data) {
    struct iq_route *route = calloc(1, sizeof(*route));
    check_mem(route);

    route->ns = strdup(ns);
    check_mem(route->ns);
    route->cb = cb;
    route->data = data;

    return route;
}

static void iq_route_del(struct iq_route *route) {
    free(route->ns);
    free(route);
}

static int iq_route_cmp(const struct iq_route *a, const struct iq_route *b) {
    int rv = strcmp(a->ns, b->ns);
    if (rv != 0) {
        return rv;
    }
    if (a->cb != b->cb) {
        return a->cb - b->cb;
    }
    if (a->data != b->data) {
        return a->data - b->data;
    }
    return 0;
}
