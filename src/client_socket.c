/**
 * xmp3 - XMPP Proxy
 * client_socket.{c,h} - Abstracts basic socket interactions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "client_socket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "log.h"

/** Data structure used for normal client_sockets. */
struct fd_data {
    /** The connected file descriptor. */
    int fd;
};

/** Data structure used for SSL client_sockets. */
struct ssl_data {
    /** The connected file descriptor. */
    int fd;

    /** The OpenSSL connection structure. */
    SSL *ssl;
};

// Forward declarations
static ssize_t fd_data_send(struct client_socket *socket, const void *buf,
                            size_t len);
static ssize_t fd_data_recv(struct client_socket *socket, void *buf,
                            size_t len);
static void fd_data_close(struct client_socket *socket);
static void fd_data_del(struct client_socket *socket);

static ssize_t ssl_data_send(struct client_socket *socket, const void *buf,
                            size_t len);
static ssize_t ssl_data_recv(struct client_socket *socket, void *buf,
                            size_t len);
static void ssl_data_close(struct client_socket *socket);
static void ssl_data_del(struct client_socket *socket);

ssize_t client_socket_send(struct client_socket *socket, const void *buf,
                           size_t len) {
    return socket->send_func(socket, buf, len);
}

ssize_t client_socket_recv(struct client_socket *socket, void *buf,
                           size_t len) {
    return socket->recv_func(socket, buf, len);
}

void client_socket_close(struct client_socket *socket) {
    socket->close_func(socket);
}

void client_socket_del(struct client_socket *socket) {
    socket->del_func(socket);
    free(socket);
}

struct client_socket* client_socket_new(int fd) {
    struct client_socket *socket = calloc(1, sizeof(*socket));
    check_mem(socket);

    struct fd_data *fd_data = calloc(1, sizeof(*fd_data));
    check_mem(fd_data);

    fd_data->fd = fd;

    socket->data = fd_data;
    socket->send_func = fd_data_send;
    socket->recv_func = fd_data_recv;
    socket->close_func = fd_data_close;
    socket->del_func = fd_data_del;

    return socket;
}

struct client_socket* client_socket_ssl_new(
        SSL_CTX *ssl_context, struct client_socket *orig_socket) {

    struct fd_data *fd_data = (struct fd_data*)orig_socket->data;

    struct client_socket *ssl_socket = calloc(1, sizeof(*ssl_socket));
    check_mem(ssl_socket);

    struct ssl_data *ssl_data = calloc(1, sizeof(*ssl_data));
    check_mem(ssl_data);

    ssl_data->fd = fd_data->fd;
    ssl_data->ssl = SSL_new(ssl_context);
    if (ssl_data->ssl == NULL) {
        ERR_print_errors_fp(stderr);
        // TODO: This should not exit, but return NULL.
        exit(1);
    }
    if (SSL_set_fd(ssl_data->ssl, ssl_data->fd) == 0) {
        ERR_print_errors_fp(stderr);
        // TODO: This should not exit, but return NULL.
        exit(1);
    }

    // TODO: This should be done elsewhere, since it could block for awhile.
    if (SSL_accept(ssl_data->ssl) != 1) {
        ERR_print_errors_fp(stderr);
        // TODO: This should not exit, but return NULL.
        exit(1);
    }
    debug("Done!");

    ssl_socket->data = ssl_data;
    ssl_socket->send_func = ssl_data_send;
    ssl_socket->recv_func = ssl_data_recv;
    ssl_socket->close_func = ssl_data_close;
    ssl_socket->del_func = ssl_data_del;

    return ssl_socket;
}

static ssize_t fd_data_send(struct client_socket *socket, const void *buf,
                            size_t len) {
    struct fd_data *fd_data = (struct fd_data*)socket->data;
    return send(fd_data->fd, buf, len, 0);
}

static ssize_t fd_data_recv(struct client_socket *socket, void *buf,
                            size_t len) {
    struct fd_data *fd_data = (struct fd_data*)socket->data;
    return recv(fd_data->fd, buf, len, 0);
}

static void fd_data_close(struct client_socket *socket) {
    struct fd_data *fd_data = (struct fd_data*)socket->data;
    shutdown(fd_data->fd, SHUT_RDWR);
    close(fd_data->fd);
}

static void fd_data_del(struct client_socket *socket) {
    struct fd_data *fd_data = (struct fd_data*)socket->data;
    free(fd_data);
}

static ssize_t ssl_data_send(struct client_socket *socket, const void *buf,
                            size_t len) {
    struct ssl_data *ssl_data = (struct ssl_data*)socket->data;
    return SSL_write(ssl_data->ssl, buf, len);
}

static ssize_t ssl_data_recv(struct client_socket *socket, void *buf,
                            size_t len) {
    struct ssl_data *ssl_data = (struct ssl_data*)socket->data;
    return SSL_read(ssl_data->ssl, buf, len);
}

static void ssl_data_close(struct client_socket *socket) {
    struct ssl_data *ssl_data = (struct ssl_data*)socket->data;
    if (SSL_shutdown(ssl_data->ssl) != 1) {
        ERR_print_errors_fp(stderr);
    }
}

static void ssl_data_del(struct client_socket *socket) {
    struct ssl_data *ssl_data = (struct ssl_data*)socket->data;
    SSL_free(ssl_data->ssl);
    free(ssl_data);
}
