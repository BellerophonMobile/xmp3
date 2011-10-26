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

static void del_client_info(struct client_info *info) {
    if (info == NULL) {
        return;
    }
    if (info->fd != -1) {
        if (close(info->fd) != 0) {
            log_err("Error closing file descriptor");
        }
    }
    XML_ParserFree(info->parser);
    free(info->jid.local);
    free(info->jid.resource);
    free(info);
}

static struct client_info* new_client_info() {
    struct client_info *info = calloc(1, sizeof(struct client_info));
    check_mem(info);

    info->authenticated = false;
    info->connected = true;

    // Create the XML parser we'll use to parse messages from the client.
    info->parser = XML_ParserCreateNS(NULL, ' ');
    check(info->parser != NULL, "Error creating XML parser");

    return info;

error:
    del_client_info(info);
    return NULL;
}

static void read_client(struct event_loop *loop, int fd, void *data) {
    struct client_info *info = (struct client_info *)data;

    ssize_t numrecv = recv(fd, MSG_BUFFER, sizeof(MSG_BUFFER), 0);

    if (numrecv == 0 || numrecv == -1) {
        switch (numrecv) {
            case 0:
                log_info("%s:%d disconnected", inet_ntoa(info->caddr.sin_addr),
                         info->caddr.sin_port);
                break;
            case -1:
                log_err("Error reading from %s:%d: %s",
                        inet_ntoa(info->caddr.sin_addr), info->caddr.sin_port,
                        strerror(errno));
                break;
        }
        goto error;
    }

    log_info("%s:%d - Read %ld bytes", inet_ntoa(info->caddr.sin_addr),
             info->caddr.sin_port, numrecv);
    enum XML_Status status = XML_Parse(info->parser, MSG_BUFFER, numrecv,
                                       false);
    check(status != XML_STATUS_ERROR, "Error parsing XML: %s",
          XML_ErrorString(XML_GetErrorCode(info->parser)));

    if (!info->connected) {
        XML_Parse(info->parser, NULL, 0, true);
        goto error;
    }

    return;

error:
    del_client_info(info);
    event_deregister_callback(loop, fd);
}

static void new_connection(struct event_loop *loop, int fd, void *data) {
    struct client_info *info = new_client_info();

    socklen_t addrlen = sizeof(info->caddr);
    info->fd = accept(fd, (struct sockaddr*)&info->caddr, &addrlen);
    check(info->fd != -1, "Error accepting client connection");

    xmpp_core_set_handlers(info->parser);
    XML_SetUserData(info->parser, info);

    log_info("New connection from %s:%d", inet_ntoa(info->caddr.sin_addr),
             info->caddr.sin_port);
    event_register_callback(loop, info->fd, read_client, info);
    return;

error:
    del_client_info(info);
}

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

    event_register_callback(loop, fd, new_connection, NULL);
    return true;

error:
    close(fd);
    return false;
}
