/**
 * xmp3 - XMPP Proxy
 * client_socket.{c,h} - Abstracts basic socket interactions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "utstring.h"

#include "log.h"

#include "client_socket.h"

struct client_socket {
    /** The connected file descriptor. */
    int fd;

    /** The address of the connected socket. */
    struct sockaddr_in addr;

    /** The OpenSSL connection structure. */
    SSL *ssl;
};

struct client_socket* client_socket_new(int fd, struct sockaddr_in addr) {
    struct client_socket *socket = calloc(1, sizeof(*socket));
    socket->fd = fd;
    socket->addr = addr;

    return socket;
}

void client_socket_del(struct client_socket *socket) {
    client_socket_close(socket);
    if (socket->ssl) {
        SSL_free(socket->ssl);
    }
    free(socket);
}

void client_socket_close(struct client_socket *socket) {
    if (socket->ssl) {
        if (SSL_shutdown(socket->ssl) != 1) {
            ERR_print_errors_fp(stderr);
        }
    }
    if (shutdown(socket->fd, SHUT_RDWR) == -1) {
        log_err("Unable to shut down client socket.");
    } else if (close(socket->fd) == -1) {
        log_err("Unable to close client socket");
    }
}

int client_socket_fd(const struct client_socket *socket) {
    return socket->fd;
}

ssize_t client_socket_send(struct client_socket *socket, const void *buf,
                           size_t len) {
    if (socket->ssl) {
        return SSL_write(socket->ssl, buf, len);
    } else {
        return send(socket->fd, buf, len, 0);
    }
}

ssize_t client_socket_recv(struct client_socket *socket, void *buf,
                           size_t len) {
    if (socket->ssl) {
        return SSL_read(socket->ssl, buf, len);
    } else {
        return recv(socket->fd, buf, len, 0);
    }
}

bool client_socket_set_ssl(struct client_socket *socket, SSL_CTX *ssl_context)
{
    if (socket->ssl) {
        SSL_free(socket->ssl);
    }

    socket->ssl = SSL_new(ssl_context);

    check(socket->ssl != NULL, "Cannot create new SSL structure.");
    check(SSL_set_fd(socket->ssl, socket->fd) != 0,
          "Unable to attach original socket to SSL structure.");
    // TODO: This should be done elsewhere, since it could block for awhile.
    check(SSL_accept(socket->ssl) == 1, "SSL_accept failed.");

    return true;

error:
    if (socket->ssl) {
        ERR_print_errors_fp(stderr);
        SSL_free(socket->ssl);
        socket->ssl = NULL;
    }
    return false;
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

int client_socket_sendxml(struct client_socket *socket, XML_Parser parser) {
    int offset, size;
    // Gets us Expat's buffer
    const char *buffer = XML_GetInputContext(parser, &offset, &size);
    check_mem(buffer);
    // Returns how much of the buffer is about the current event
    int count = XML_GetCurrentByteCount(parser);

    return client_socket_sendall(socket, buffer + offset, count);
}

char* client_socket_addr_str(struct client_socket *socket) {
    UT_string s;
    utstring_init(&s);

    utstring_printf(&s, "%s:%d", inet_ntoa(socket->addr.sin_addr),
                    socket->addr.sin_port);

    return utstring_body(&s);
}
