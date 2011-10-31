/**
 * xmp3 - XMPP Proxy
 * xmpp.{c,h} - Implements the server part of a normal XMPP server.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <expat.h>

#include "utlist.h"

#include "log.h"
#include "utils.h"
#include "event.h"

#include "xmpp_common.h"
#include "xmpp_core.h"

#define BUFFER_SIZE 2000

static const int SERVER_BACKLOG = 3;

static char MSG_BUFFER[BUFFER_SIZE];

// Forward declarations
static struct xmpp_server* new_server();
static void del_server(struct xmpp_server *server);
static struct xmpp_client* new_client(struct xmpp_server *server);
static void del_client(struct xmpp_client *client);

static void add_connection(struct event_loop *loop, int fd, void *data);
static void remove_connection(struct xmpp_server *server,
                              struct xmpp_client *client);

static struct message_route* new_message_route(struct jid *jid,
        xmpp_message_route route_func, void *data);
static void del_message_route(struct message_route *route);

static void read_client(struct event_loop *loop, int fd, void *data);

bool xmpp_init(struct event_loop *loop, struct in_addr addr, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    check(fd != -1, "Error creating XMPP server socket");

    // Allow address reuse when in the TIME_WAIT state.
    static const int on = 1;
    check(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != -1,
          "Error setting SO_REUSEADDR on server socket");

    // Convert to network byte order
    struct sockaddr_in saddr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = addr,
    };

    check(bind(fd, (struct sockaddr*)&saddr, sizeof(saddr)) != -1,
          "XMPP server socket bind error");
    check(listen(fd, SERVER_BACKLOG) != -1, "XMPP server socket listen error");

    log_info("Listening for XMPP connections on %s:%d", inet_ntoa(addr), port);

    struct xmpp_server *server = new_server();

    event_register_callback(loop, fd, add_connection, server);
    return true;

error:
    close(fd);
    return false;
}

void xmpp_register_message_route(struct xmpp_server *server, struct jid *jid,
                                 xmpp_message_route route_func, void *data) {
    struct message_route *route = xmpp_find_message_route(server, jid);
    if (route != NULL) {
        log_warn("Attempted to insert duplicate route");
        return;
    }
    DL_APPEND(server->message_routes,
              new_message_route(jid, route_func, data));
}

void xmpp_deregister_message_desitnation(struct xmpp_server *server,
                                         struct jid *jid) {
    struct message_route *route = xmpp_find_message_route(server, jid);
    if (route == NULL) {
        log_warn("Attempted to remove non-existent key");
        return;
    }
    DL_DELETE(server->message_routes, route);
    del_message_route(route);
}

struct message_route* xmpp_find_message_route(struct xmpp_server *server,
                                              struct jid *jid) {
    struct message_route *route;
    DL_FOREACH(server->message_routes, route) {
        if (strcmp(route->jid->local, jid->local) == 0
            && strcmp(route->jid->domain, jid->domain) == 0) {
            /* If no resource specified in search, then return the first one
             * that matches the local and domain parts, else return an exact
             * match. */
            if (jid->resource == NULL
                || strcmp(route->jid->resource, jid->resource) == 0) {
                return route;
            }
        }
    }
    return NULL;
}

static struct xmpp_server* new_server() {
    struct xmpp_server *server = calloc(1, sizeof(*server));
    check_mem(server);
    LIST_INIT(&server->client_list);
    return server;
}

static void del_server(struct xmpp_server *server) {
    struct xmpp_client *item;
    LIST_FOREACH(item, &server->client_list, list_entry) {
        del_client(item);
    }
    free(server);
}

static struct xmpp_client* new_client(struct xmpp_server *server) {
    struct xmpp_client *client = calloc(1, sizeof(*client));
    check_mem(client);

    client->authenticated = false;
    client->connected = true;
    client->server = server;

    // Create the XML parser we'll use to parse messages from the client.
    client->parser = XML_ParserCreateNS(NULL, ' ');
    check(client->parser != NULL, "Error creating XML parser");

    return client;

error:
    del_client(client);
    return NULL;
}

static void del_client(struct xmpp_client *client) {
    if (client == NULL) {
        return;
    }
    if (client->fd != -1) {
        if (close(client->fd) != 0) {
            log_err("Error closing file descriptor");
        }
    }
    XML_ParserFree(client->parser);
    free(client->jid.local);
    free(client->jid.resource);
    free(client);
}

static void read_client(struct event_loop *loop, int fd, void *data) {
    struct xmpp_client *client = (struct xmpp_client*)data;
    struct xmpp_server *server = client->server;

    ssize_t numrecv = recv(fd, MSG_BUFFER, sizeof(MSG_BUFFER), 0);

    if (numrecv == 0 || numrecv == -1) {
        switch (numrecv) {
            case 0:
                log_info("%s:%d disconnected",
                         inet_ntoa(client->caddr.sin_addr),
                                   client->caddr.sin_port);
                break;
            case -1:
                log_err("Error reading from %s:%d: %s",
                        inet_ntoa(client->caddr.sin_addr),
                        client->caddr.sin_port, strerror(errno));
                break;
        }
        goto error;
    }

    log_info("%s:%d - Read %ld bytes", inet_ntoa(client->caddr.sin_addr),
             client->caddr.sin_port, numrecv);
    xmpp_print_data(MSG_BUFFER, numrecv);
    enum XML_Status status = XML_Parse(client->parser, MSG_BUFFER,
                                       numrecv, false);
    check(status != XML_STATUS_ERROR, "Error parsing XML: %s",
          XML_ErrorString(XML_GetErrorCode(client->parser)));

    if (!client->connected) {
        XML_Parse(client->parser, NULL, 0, true);
        goto error;
    }

    return;

error:
    event_deregister_callback(loop, fd);
    remove_connection(server, client);
}

static void add_connection(struct event_loop *loop, int fd, void *data) {
    struct xmpp_server *server = (struct xmpp_server*)data;
    struct xmpp_client *client = new_client(server);

    socklen_t addrlen = sizeof(client->caddr);
    client->fd = accept(fd, (struct sockaddr*)&client->caddr, &addrlen);
    check(client->fd != -1, "Error accepting client connection");

    xmpp_core_set_handlers(client->parser);
    XML_SetUserData(client->parser, client);

    log_info("New connection from %s:%d",
             inet_ntoa(client->caddr.sin_addr), client->caddr.sin_port);

    LIST_INSERT_HEAD(&server->client_list, client, list_entry);

    event_register_callback(loop, client->fd, read_client, client);
    return;

error:
    del_client(client);
}

static void remove_connection(struct xmpp_server *server,
                              struct xmpp_client *client) {
    struct xmpp_client *item;
    LIST_FOREACH(item, &server->client_list, list_entry) {
        if (client == item) {
            LIST_REMOVE(item, list_entry);
            del_client(item);
            return;
        }
    }
}

static struct message_route* new_message_route(struct jid *jid,
        xmpp_message_route route_func, void *data) {
    struct message_route *route = calloc(1, sizeof(*route));
    check_mem(route);
    route->jid = jid;
    route->route_func = route_func;
    route->data = data;
    return route;
}

static void del_message_route(struct message_route *route) {
    free(route);
}
