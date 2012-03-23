/*
 * Copyright (c) 2012 Tom Wambold <tom5760@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file xmpp_server.c
 * Main XMPP server data/functions
 */

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "uthash.h"
#include "utlist.h"

#include "log.h"

#include "client_socket.h"
#include "event.h"
#include "jid.h"
#include "utils.h"
#include "xmp3_options.h"
#include "xmpp_client.h"
#include "xmpp_core.h"
#include "xmpp_im.h"
#include "xmpp_parser.h"
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
    type ## _del(search); \
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

/** Holds data for items to report during a DISCO items query. */
struct disco_item {
    /** The "name" attribute for the response. */
    char *name;

    /** The "jid" attribute for the response. */
    struct jid *jid;

    /** @{ These are kept in a doubly-linked list. */
    struct disco_item *prev;
    struct disco_item *next;
    /** @} */
};

/** Holds data for an authentication callback. */
struct auth_callback {
    /** The function to call for authentication. */
    xmpp_server_auth_callback cb;

    /** A function to delete the data when finished. */
    void (*del)(void*);

    /** Data that is passed to the callback when it is called. */
    void *data;
};

/** Simple structure to allow users to iterate over connected clients. */
struct xmpp_client_iterator {
    struct c_client *client;
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

    /** The list of items to report for a disco items query. */
    struct disco_item *disco_items;

    /** The currently configured authentication callback. */
    struct auth_callback auth_callback;
};

/* Forward declarations. */
static bool init_socket(struct xmpp_server *server,
                        const struct xmp3_options *options);
static bool init_ssl(struct xmpp_server *server,
                     const struct xmp3_options *options);

static void connect_client(struct event_loop *loop, int fd, void *data);
static void read_client(struct event_loop *loop, int fd, void *data);

static void send_service_unavailable(struct xmpp_server *server,
                                     struct xmpp_stanza *stanza);

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

static void disco_item_del(struct disco_item *item);

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

    /* Set up inital stanza and IQ routes. */
    xmpp_server_add_stanza_route(server, server->jid,
                                 xmpp_core_route_server, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_SESSION_NS,
                             xmpp_im_iq_session, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_DISCO_ITEMS_NS,
                             xmpp_im_iq_disco_items, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_DISCO_INFO_NS,
                             xmpp_im_iq_disco_info, NULL);
    xmpp_server_add_iq_route(server, XMPP_IQ_ROSTER_NS,
                             xmpp_im_iq_roster, NULL);

    /* Register the event handler so we can get notified of new connections. */
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
    struct c_client *connected_client = NULL;
    struct c_client *connected_client_tmp = NULL;

    if (server->auth_callback.data != NULL
            && server->auth_callback.del != NULL) {
        server->auth_callback.del(server->auth_callback.data);
    }

    DL_FOREACH_SAFE(server->clients, connected_client, connected_client_tmp) {
        DL_DELETE(server->clients, connected_client);
        xmpp_client_del(connected_client->client);
        free(connected_client);
    }

    DELETE_LIST(stanza_route, server->stanza_routes);
    DELETE_LIST(iq_route, server->iq_routes);
    DELETE_LIST(client_listener, server->client_listeners);
    DELETE_LIST(disco_item, server->disco_items);

    if (server->jid) {
        jid_del(server->jid);
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

void xmpp_server_set_auth_callback(struct xmpp_server *server,
                                   xmpp_server_auth_callback cb,
                                   void (*del)(void*),
                                   void *data) {
    if (server->auth_callback.data != NULL
            && server->auth_callback.del != NULL) {
        server->auth_callback.del(server->auth_callback.data);
    }
    server->auth_callback.cb = cb;
    server->auth_callback.del = del;
    server->auth_callback.data = data;
}

bool xmpp_server_authenticate(const struct xmpp_server *server,
                              const char *authzid, const char *authcid,
                              const char *password) {
    if (server->auth_callback.cb == NULL) {
        return true;
    }
    return server->auth_callback.cb(authzid, authcid, password,
                                    server->auth_callback.data);
}

void xmpp_server_add_client_listener(struct xmpp_client *client,
                                     xmpp_server_client_callback cb,
                                     void *data) {
    struct xmpp_server *server = xmpp_client_server(client);
    char *strjid = jid_to_str(xmpp_client_jid(client));
    debug("Registering disconnect listener for '%s'", strjid);
    free(strjid);
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

    /* Sanity check that the client is registered with the server. */
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

    DL_DELETE(server->clients, search);

    struct client_listener *listener = NULL;
    struct client_listener *tmp = NULL;
    DL_FOREACH_SAFE(server->client_listeners, listener, tmp) {
        if (listener->client == client) {
            listener->cb(client, listener->data);
            DL_DELETE(server->client_listeners, listener);
            free(listener);
        }
    }

    xmpp_client_del(client);
    free(search);
}

struct xmpp_client* xmpp_server_find_client(const struct xmpp_server *server,
                                            const struct jid *jid) {
    struct c_client *search = NULL;
    DL_FOREACH(server->clients, search) {
        if (jid_cmp(jid, xmpp_client_jid(search->client)) == 0) {
            return search->client;
        }
    }
    return NULL;
}

struct xmpp_client_iterator* xmpp_client_iterator_new(
        const struct xmpp_server *server) {
    struct xmpp_client_iterator *iter = calloc(1, sizeof(*iter));
    check_mem(iter);

    iter->client = server->clients;
    return iter;
}

struct xmpp_client* xmpp_client_iterator_next(
        struct xmpp_client_iterator *iter) {
    if (iter->client == NULL) {
        return NULL;
    }
    struct xmpp_client *rv = iter->client->client;
    iter->client = iter->client->next;
    return rv;
}

void xmpp_client_iterator_del(struct xmpp_client_iterator *iter) {
    free(iter);
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

bool xmpp_server_route_stanza(struct xmpp_server *server,
                              struct xmpp_stanza *stanza) {
    struct jid *search_jid = jid_new_from_str(xmpp_stanza_attr(
                stanza, XMPP_STANZA_ATTR_TO));

    char *strjid = jid_to_str(search_jid);
    debug("Searching for route to: '%s'", strjid);
    free(strjid);

    bool was_handled = false;
    struct stanza_route *route = NULL;
    DL_FOREACH(server->stanza_routes, route) {
        strjid = jid_to_str(route->jid);
        debug("Is it '%s'", strjid);
        free(strjid);
        if (jid_cmp_wildcards(search_jid, route->jid) == 0) {
            debug("Yes");
            if (route->cb(stanza, server, route->data)) {
                was_handled = true;
            }
        } else {
            debug("No");
        }
    }
    if (!was_handled) {
        log_info("No route for destination");
    }
    jid_del(search_jid);
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

bool xmpp_server_route_iq(struct xmpp_server *server,
                          struct xmpp_stanza *stanza) {

    bool was_handled = false;
    check(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID) != NULL,
          "IQ stanza without ID");
    check(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_TYPE) != NULL,
          "IQ stanza without type");
    check(xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM) != NULL,
          "IQ stanza without from");

    struct xmpp_stanza *child = xmpp_stanza_children(stanza);

    if (child == NULL) {
        log_err("IQ stanza has no child");
        return false;
    }

    const char *search_uri = xmpp_stanza_uri(child);
    debug("Searching for IQ namespace: %s", search_uri);

    struct iq_route *route = NULL;
    DL_FOREACH(server->iq_routes, route) {
        debug("Is it '%s'?", route->ns);
        /* These strings come from Expat, so they should be null-terminated. */
        if (strcmp(search_uri, route->ns) == 0) {
            debug("Yes!");
            if (route->cb(stanza, server, route->data)) {
                was_handled = true;
            }
        }
    }

error:
    if (!was_handled) {
        log_info("No route for destination");
        send_service_unavailable(server, stanza);
    }
    return was_handled;
}

void xmpp_server_add_disco_item(struct xmpp_server *server,
                                const char *name, const struct jid *jid) {
    struct disco_item *item = calloc(1, sizeof(*item));
    check_mem(item);

    STRDUP_CHECK(item->name, name);
    item->jid = jid_new_from_jid(jid);

    DL_APPEND(server->disco_items, item);
}

void xmpp_server_del_disco_item(struct xmpp_server *server,
                                const char *name, const struct jid *jid) {
    struct disco_item *item;
    DL_FOREACH(server->disco_items, item) {
        /* These strings come from Expat, so they should be null-terminated. */
        if (strcmp(name, item->name) == 0 && jid_cmp(jid, item->jid) == 0) {
            DL_DELETE(server->disco_items, item);
            disco_item_del(item);
            return;
        }
    }
}

void xmpp_server_append_disco_items(struct xmpp_server *server,
                                    struct xmpp_stanza *stanza) {
    struct disco_item *item;
    DL_FOREACH(server->disco_items, item) {
        char *strjid = jid_to_str(item->jid);
        struct xmpp_stanza *item_stanza = xmpp_stanza_new("item", (const char*[]){
                "name", item->name,
                "jid", strjid,
                NULL,});
        xmpp_stanza_append_child(stanza, item_stanza);
        free(strjid);
    }
}

/** Simple function to create and bind the server socket. */
static bool init_socket(struct xmpp_server *server,
                        const struct xmp3_options *options) {
    server->fd = socket(AF_INET, SOCK_STREAM, 0);
    check(server->fd != -1, "Error creating XMPP server socket");

    /* Allow address reuse when in the TIME_WAIT state. */
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

/** Initializes the OpenSSL context, setting up the key and certificate. */
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

/**
 * Callback for new client connections.
 *
 * Handles accepting the connection, and setting up XMPP stream parsing.
 */
static void connect_client(struct event_loop *loop, int fd, void *data) {
    struct xmpp_server *server = (struct xmpp_server*)data;
    struct client_socket *socket = NULL;
    struct xmpp_client *client = NULL;
    struct c_client *connected_client = NULL;

    struct sockaddr_in caddr;
    socklen_t caddrlen = sizeof(caddr);

    int client_fd = accept(fd, (struct sockaddr*)&caddr, &caddrlen);
    check(client_fd != -1, "Error accepting new client connection");

    socket = client_socket_new(client_fd, caddr);
    check_mem(socket);

    client = xmpp_client_new(server, socket);
    check_mem(client);

    event_register_callback(loop, client_fd, read_client, client);

    connected_client = calloc(1, sizeof(*connected_client));
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

/**
 * Callback for new data from a client.
 *
 * Handles feeding the data to the XML parser.
 */
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

    struct xmpp_parser *parser = xmpp_client_parser(client);
    check(xmpp_parser_parse(parser, server->buffer, numrecv),
          "Error parsing XML: %s", xmpp_parser_strerror(parser));
    return;

error:
    xmpp_server_disconnect_client(client);
}

/**
 * Sends a <service-unavailable> error stanza to a client.
 *
 * Sent when there is no handler for an IQ stanza.
 */
static void send_service_unavailable(struct xmpp_server *server,
                                     struct xmpp_stanza *stanza) {
    log_info("Sending service unavailable.");

    const char *id = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_ID);
    check(id != NULL, "Disco IQ needs id attribute");

    const char *from = xmpp_stanza_attr(stanza, XMPP_STANZA_ATTR_FROM);
    check(from != NULL, "Disco IQ needs from attribute");

    struct xmpp_stanza *response = xmpp_stanza_new("iq", (const char*[]){
            XMPP_STANZA_ATTR_ID, id,
            XMPP_STANZA_ATTR_FROM, "localhost",
            XMPP_STANZA_ATTR_TO, from,
            XMPP_STANZA_ATTR_TYPE, XMPP_STANZA_TYPE_ERROR,
            NULL});

    struct xmpp_stanza *error = xmpp_stanza_new("error", (const char*[]){
            "type", "cancel",
            NULL});
    xmpp_stanza_append_child(response, error);

    struct xmpp_stanza *unavail = xmpp_stanza_new("service-unavailable",
            (const char*[]){"xmlns", XMPP_STANZA_NS_STANZA, NULL});
    xmpp_stanza_append_child(error, unavail);

    xmpp_server_route_stanza(server, response);
    xmpp_stanza_del(response, true);

error:
    return;
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
    /* These URIs come from Expat, so they should be null-terminated. */
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

static void disco_item_del(struct disco_item *item) {
    jid_del(item->jid);
    free(item->name);
    free(item);
}
