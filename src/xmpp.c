/**
 * xmp3 - XMPP Proxy
 * xmpp.{c,h} - Implements the server part of a normal XMPP server.
 * Copyright (c) 2011 Drexel University
 */

#include "xmpp.h"

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "event.h"

static const int SERVER_BACKLOG = 3;

static void xmpp_new_connection(struct event_loop *loop, int fd, void *data) {
    printf("New connection!\n");

    int newfd = accept(fd, NULL, NULL);
    shutdown(newfd, SHUT_RDWR);
    close(newfd);
}

bool xmpp_init(struct event_loop *loop, struct in_addr addr, uint16_t port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Error creating XMPP server socket");
        return false;
    }

    // Convert to network byte order
    struct sockaddr_in saddr = { AF_INET, htons(port), addr };

    if (bind(fd, (struct sockaddr*)&saddr, sizeof(saddr))) {
        perror("XMPP server socket bind error");
        return false;
    }

    if (listen(fd, SERVER_BACKLOG)) {
        perror("XMPP server socket listen error");
        return false;
    }

    printf("Listening for XMPP connections on %s:%d\n", inet_ntoa(addr), port);

    event_register_callback(loop, fd, xmpp_new_connection, NULL);
    return true;
}
