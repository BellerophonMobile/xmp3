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

#include "event.h"

#define BUFFER_SIZE 2000

static const int SERVER_BACKLOG = 3;

static char MSG_BUFFER[BUFFER_SIZE];

struct client_info {
    struct sockaddr_in caddr;
};

static void xmpp_read_client(struct event_loop *loop, int fd, void *data) {
    struct client_info *info = (struct client_info *)data;

    ssize_t numrecv = recv(fd, MSG_BUFFER, sizeof(MSG_BUFFER), 0);

    if (numrecv == 0 || numrecv == -1) {
        switch (numrecv) {
            case 0:
                printf("%s:%d disconnected\n", inet_ntoa(info->caddr.sin_addr),
                       info->caddr.sin_port);
                break;
            case -1:
                fprintf(stderr, "Error reading from %s:%d: %s",
                        inet_ntoa(info->caddr.sin_addr), info->caddr.sin_port,
                        strerror(errno));
                break;
        }
        event_deregister_callback(loop, fd);
        close(fd);
        return;
    }

    printf("%s:%d(%d bytes): %.*3$s\n", inet_ntoa(info->caddr.sin_addr),
           info->caddr.sin_port, numrecv, MSG_BUFFER);
}

static void xmpp_new_connection(struct event_loop *loop, int fd, void *data) {
    struct client_info *info = malloc(sizeof(struct client_info));
    if (info == NULL) {
        fprintf(stderr, "Error allocating connection structure.\n");
        abort();
    }

    socklen_t addrlen = sizeof(info->caddr);
    int clientfd = accept(fd, (struct sockaddr*)&info->caddr, &addrlen);
    printf("New connection from %s:%d\n", inet_ntoa(info->caddr.sin_addr),
           info->caddr.sin_port);
    event_register_callback(loop, clientfd, xmpp_read_client, info);
}

bool xmpp_init(struct event_loop *loop, struct in_addr addr, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Error creating XMPP server socket");
        return false;
    }

    // Allow address reuse when in the TIME_WAIT state.
    static const int on = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        perror("Error setting SO_REUSEADDR on server socket");
        goto error;
    }

    // Convert to network byte order
    struct sockaddr_in saddr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = addr,
    };

    if (bind(fd, (struct sockaddr*)&saddr, sizeof(saddr))) {
        perror("XMPP server socket bind error");
        goto error;
    }

    if (listen(fd, SERVER_BACKLOG)) {
        perror("XMPP server socket listen error");
        goto error;
    }

    printf("Listening for XMPP connections on %s:%d\n", inet_ntoa(addr), port);

    event_register_callback(loop, fd, xmpp_new_connection, NULL);
    return true;

error:
    close(fd);
    return false;
}
