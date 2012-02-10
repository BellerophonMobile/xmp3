/**
 * xmp3 - XMPP Proxy
 * client_socket.{c,h} - Abstracts basic socket interactions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "utstring.h"
#include "log.h"

#include "client_socket.h"

struct fd_socket {
    /** The connected file descriptor. */
    int fd;

    /** The address of the connected socket. */
    struct sockaddr_in addr;
};

struct ssl_socket {
    struct fd_socket *fd_socket;

    /** The OpenSSL connection structure. */
    SSL *ssl;
};

// Forward declarations
static void fd_del(struct client_socket *socket);
static void fd_close(struct client_socket *socket);
static int fd_fd(struct client_socket *socket);
static ssize_t fd_send(struct client_socket *socket, const void *buf,
                       size_t len);
static ssize_t fd_recv(struct client_socket *socket, void *buf, size_t len);
static char* fd_str(struct client_socket *socket);

static void ssl_del(struct client_socket *socket);
static void ssl_close(struct client_socket *socket);
static int ssl_fd(struct client_socket *socket);
static ssize_t ssl_send(struct client_socket *socket, const void *buf,
                        size_t len);
static ssize_t ssl_recv(struct client_socket *socket, void *buf, size_t len);
static char* ssl_str(struct client_socket *socket);

struct client_socket* client_socket_new(int fd, struct sockaddr_in addr) {
    struct client_socket *socket = calloc(1, sizeof(*socket));
    check_mem(socket);

    struct fd_socket *self = calloc(1, sizeof(*self));
    check_mem(self);
    self->fd = fd;
    self->addr = addr;

    socket->self = self;
    socket->del_func = fd_del;
    socket->close_func = fd_close;
    socket->fd_func = fd_fd;
    socket->send_func = fd_send;
    socket->recv_func = fd_recv;
    socket->str_func = fd_str;

    return socket;
}

struct client_socket* client_socket_ssl_new(struct client_socket *socket,
                                            SSL_CTX *ssl_context) {
    struct ssl_socket *self = calloc(1, sizeof(*self));
    check_mem(self);

    self->fd_socket = (struct fd_socket*)socket->self;

    self->ssl = SSL_new(ssl_context);
    check(self->ssl != NULL, "Cannot create new SSL structure.");
    check(SSL_set_fd(self->ssl, self->fd_socket->fd) != 0,
          "Unable to attach original socket to SSL structure.");
    // TODO: This should be done elsewhere, since it could block for awhile.
    check(SSL_accept(self->ssl) == 1, "SSL_accept failed.");

    socket->self = self;
    socket->del_func = ssl_del;
    socket->close_func = ssl_close;
    socket->fd_func = ssl_fd;
    socket->send_func = ssl_send;
    socket->recv_func = ssl_recv;
    socket->str_func = ssl_str;

    return socket;

error:
    ERR_print_errors_fp(stderr);
    SSL_free(self->ssl);
    free(self->fd_socket);
    free(self);
    free(socket);
    return NULL;
}

void client_socket_del(struct client_socket *socket) {
    socket->del_func(socket);
}

void client_socket_close(struct client_socket *socket) {
    socket->close_func(socket);
}

int client_socket_fd(struct client_socket *socket) {
    return socket->fd_func(socket);
}

ssize_t client_socket_send(struct client_socket *socket, const void *buf,
                           size_t len) {
    return socket->send_func(socket, buf, len);
}

ssize_t client_socket_recv(struct client_socket *socket, void *buf,
                           size_t len) {
    return socket->recv_func(socket, buf, len);
}

ssize_t client_socket_sendall(struct client_socket *socket, const void *buf,
                              size_t len) {
    // Keep track of how much we've sent so far
    ssize_t numsent = 0;
    do {
        ssize_t newsent = client_socket_send(socket, buf + numsent,
                                             len - numsent);
        check(newsent != -1, "Error sending data");
        numsent += newsent;
    } while (numsent < len);

    return numsent;
error:
    return -1;
}

char* client_socket_addr_str(struct client_socket *socket) {
    return socket->str_func(socket);
}

static void fd_del(struct client_socket *socket) {
    struct fd_socket *self = (struct fd_socket*)socket->self;
    free(self);
}

static void fd_close(struct client_socket *socket) {
    struct fd_socket *self = (struct fd_socket*)socket->self;
    if (shutdown(self->fd, SHUT_RDWR) == -1) {
        log_err("Unable to shut down client socket.");
    } else if (close(self->fd) == -1) {
        log_err("Unable to close client socket");
    }
}

static int fd_fd(struct client_socket *socket) {
    struct fd_socket *self = (struct fd_socket*)socket->self;
    return self->fd;
}

static ssize_t fd_send(struct client_socket *socket, const void *buf,
                       size_t len) {
    struct fd_socket *self = (struct fd_socket*)socket->self;
    return send(self->fd, buf, len, 0);
}

static ssize_t fd_recv(struct client_socket *socket, void *buf, size_t len) {
    struct fd_socket *self = (struct fd_socket*)socket->self;
    return recv(self->fd, buf, len, 0);
}

static char* fd_str(struct client_socket *socket) {
    struct fd_socket *self = (struct fd_socket*)socket->self;
    UT_string s;
    utstring_init(&s);
    utstring_printf(&s, "%s:%d", inet_ntoa(self->addr.sin_addr),
                    self->addr.sin_port);
    return utstring_body(&s);
}

static void ssl_del(struct client_socket *socket) {
    struct ssl_socket *self = (struct ssl_socket*)socket->self;
    SSL_free(self->ssl);
    free(self->fd_socket);
    free(self);
}

static void ssl_close(struct client_socket *socket) {
    struct ssl_socket *self = (struct ssl_socket*)socket->self;

    /* If the socket has already been closed, SSL_shutdown will cause
     * a SIGPIPE to be sent to this process.  We don't care, so block it
     * every time. */
    struct sigaction sa = { .sa_handler=SIG_IGN };
    struct sigaction osa;
    if (sigaction(SIGPIPE, &sa, &osa) == 0) {
        if (SSL_shutdown(self->ssl) != 1) {
            ERR_print_errors_fp(stderr);
        }
        if (sigaction(SIGPIPE, &osa, NULL) != 0) {
            log_err("Can't unignore SIGPIPE");
        }
    } else {
        log_err("Can't ignore SIGPIPE.");
    }
}

static int ssl_fd(struct client_socket *socket) {
    struct ssl_socket *self = (struct ssl_socket*)socket->self;
    return self->fd_socket->fd;
}

static ssize_t ssl_send(struct client_socket *socket, const void *buf,
                        size_t len) {
    struct ssl_socket *self = (struct ssl_socket*)socket->self;
    return SSL_write(self->ssl, buf, len);
}

static ssize_t ssl_recv(struct client_socket *socket, void *buf, size_t len) {
    struct ssl_socket *self = (struct ssl_socket*)socket->self;
    return SSL_read(self->ssl, buf, len);
}

static char* ssl_str(struct client_socket *socket) {
    struct ssl_socket *self = (struct ssl_socket*)socket->self;
    socket->self = self->fd_socket;
    char *addrstr = fd_str(socket);
    socket->self = self;
    return addrstr;
}
