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

#include "log.h"
#include "event.h"

#include "xmpp_common.h"
#include "xmpp_core.h"

#define BUFFER_SIZE 2000

static const int SERVER_BACKLOG = 3;

static char MSG_BUFFER[BUFFER_SIZE];

struct server_info {
    int fd;
    LIST_HEAD(clients, client_info) clients_head;
};

// Forward declarations
static struct server_info* new_server_info();
static void del_server_info(struct server_info *server_info);
static struct client_info* new_client_info(struct server_info *server_info);
static void del_client_info(struct client_info *client_info);

static void add_connection(struct event_loop *loop, int fd, void *data);
static void remove_connection(struct server_info *server_info,
                              struct client_info *client_info);

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

    struct server_info *server_info = new_server_info();

    event_register_callback(loop, fd, add_connection, server_info);
    return true;

error:
    close(fd);
    return false;
}

struct client_info* xmpp_find_client(struct jid *jid,
                                     struct server_info *server_info) {
    struct client_info *item;
    LIST_FOREACH(item, &server_info->clients_head, list_entry) {
        if ((strcmp(item->jid.local, jid->local) == 0) &&
            (jid->resource == NULL ||
             strcmp(item->jid.resource, jid->resource) == 0)) {
            return item;
        }
    }
    return NULL;
}

static struct server_info* new_server_info() {
    struct server_info *server_info = calloc(1, sizeof(*server_info));
    check_mem(server_info);
    LIST_INIT(&server_info->clients_head);
    return server_info;
}

static void del_server_info(struct server_info *server_info) {
    struct client_info *item;
    LIST_FOREACH(item, &server_info->clients_head, list_entry) {
        del_client_info(item);
    }
    free(server_info);
}

static struct client_info* new_client_info(struct server_info *server_info) {
    struct client_info *client_info = calloc(1, sizeof(*client_info));
    check_mem(client_info);

    client_info->authenticated = false;
    client_info->connected = true;
    client_info->server_info = server_info;

    // Create the XML parser we'll use to parse messages from the client.
    client_info->parser = XML_ParserCreateNS(NULL, ' ');
    check(client_info->parser != NULL, "Error creating XML parser");

    return client_info;

error:
    del_client_info(client_info);
    return NULL;
}

static void del_client_info(struct client_info *client_info) {
    if (client_info == NULL) {
        return;
    }
    if (client_info->fd != -1) {
        if (close(client_info->fd) != 0) {
            log_err("Error closing file descriptor");
        }
    }
    XML_ParserFree(client_info->parser);
    free(client_info->jid.local);
    free(client_info->jid.resource);
    free(client_info);
}

static void read_client(struct event_loop *loop, int fd, void *data) {
    struct client_info *client_info = (struct client_info *)data;
    struct server_info *server_info = client_info->server_info;

    ssize_t numrecv = recv(fd, MSG_BUFFER, sizeof(MSG_BUFFER), 0);

    if (numrecv == 0 || numrecv == -1) {
        switch (numrecv) {
            case 0:
                log_info("%s:%d disconnected",
                         inet_ntoa(client_info->caddr.sin_addr),
                                   client_info->caddr.sin_port);
                break;
            case -1:
                log_err("Error reading from %s:%d: %s",
                        inet_ntoa(client_info->caddr.sin_addr),
                        client_info->caddr.sin_port, strerror(errno));
                break;
        }
        goto error;
    }

    log_info("%s:%d - Read %ld bytes", inet_ntoa(client_info->caddr.sin_addr),
             client_info->caddr.sin_port, numrecv);
    xmpp_print_data(MSG_BUFFER, numrecv);
    enum XML_Status status = XML_Parse(client_info->parser, MSG_BUFFER,
                                       numrecv, false);
    check(status != XML_STATUS_ERROR, "Error parsing XML: %s",
          XML_ErrorString(XML_GetErrorCode(client_info->parser)));

    if (!client_info->connected) {
        XML_Parse(client_info->parser, NULL, 0, true);
        goto error;
    }

    return;

error:
    event_deregister_callback(loop, fd);
    remove_connection(server_info, client_info);
}

static void add_connection(struct event_loop *loop, int fd, void *data) {
    struct server_info *server_info = (struct server_info*)data;
    struct client_info *client_info = new_client_info(server_info);

    socklen_t addrlen = sizeof(client_info->caddr);
    client_info->fd = accept(fd, (struct sockaddr*)&client_info->caddr,
                             &addrlen);
    check(client_info->fd != -1, "Error accepting client connection");

    xmpp_core_set_handlers(client_info->parser);
    XML_SetUserData(client_info->parser, client_info);

    log_info("New connection from %s:%d",
             inet_ntoa(client_info->caddr.sin_addr),
             client_info->caddr.sin_port);

    LIST_INSERT_HEAD(&server_info->clients_head, client_info, list_entry);

    event_register_callback(loop, client_info->fd, read_client, client_info);
    return;

error:
    del_client_info(client_info);
}

static void remove_connection(struct server_info *server_info,
                              struct client_info *client_info) {
    struct client_info *item;
    LIST_FOREACH(item, &server_info->clients_head, list_entry) {
        if (client_info == item) {
            LIST_REMOVE(item, list_entry);
            del_client_info(item);
            return;
        }
    }
}
