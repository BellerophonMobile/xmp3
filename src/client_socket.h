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
 * @file client_socket.h
 * Abstracts basic socket interactions.
 */

#pragma once

#include <stdbool.h>
#include <netinet/in.h>

#include <openssl/ssl.h>

struct client_socket;

typedef void (*client_socket_del_func)(struct client_socket *socket);
typedef void (*client_socket_close_func)(struct client_socket *socket);
typedef int (*client_socket_fd_func)(struct client_socket *socket);
typedef ssize_t (*client_socket_send_func)(struct client_socket *socket,
                                           const void *buffer, size_t length);
typedef ssize_t (*client_socket_recv_func)(struct client_socket *socket,
                                           void *buffer, size_t length);
typedef char* (*client_socket_str_func)(struct client_socket *socket);

struct client_socket {
    client_socket_del_func del_func;
    client_socket_close_func close_func;
    client_socket_fd_func fd_func;
    client_socket_send_func send_func;
    client_socket_recv_func recv_func;
    client_socket_str_func str_func;

    void *self;
};

/**
 * Allocates and initializes a new client socket using a file descriptor.
 *
 * Reading/writing is done just with send/recv.
 *
 * @param fd   The file descriptor to read/write from.
 * @param addr The address of the connected client.
 * @return A client_socket structure.
 */
struct client_socket* client_socket_new(int fd, struct sockaddr_in addr);

/**
 * Creates a SSL client socket from an existing normal client socket.
 *
 * The original socket is modified to support SSL.
 *
 * @param socket The socket to convert to SSL.
 * @param ssl_context The OpenSSL context.
 * @returns A pointer to the same input socket, which has been modified.
 */
struct client_socket* client_socket_ssl_new(struct client_socket *socket,
                                            SSL_CTX *ssl_context);

/** Closes, cleans up and deallocates a client_socket structure. */
void client_socket_del(struct client_socket *socket);

/** Closes a client_socket. */
void client_socket_close(struct client_socket *socket);

int client_socket_fd(struct client_socket *socket);

/**
 * Send some data to a client_socket.
 *
 * @param socket The client socket to send data to.
 * @param buf    The data to send.
 * @param len    The length of buf.
 * @return The number of bytes written, <= 0 on error.
 */
ssize_t client_socket_send(struct client_socket *socket, const void *buf,
                           size_t len);

/**
 * Receive some data from a client_socket.
 *
 * @param socket The client socket to receive data from.
 * @param buf    A buffer to write data into
 * @param len    The length of buf.
 * @return The number of bytes written, <= 0 on error.
 */
ssize_t client_socket_recv(struct client_socket *socket, void *buf,
                           size_t len);

/**
 * Send all data in a buffer to the connected socket.
 *
 * This will keep trying until all data is sent.  This should later be
 * rewritten to add a write callback using a non-blocking socket.
 *
 * @param fd     File descriptor to send to.
 * @param buffer Buffer to read data from.
 * @param len    Length of the buffer to send.
 * @return Number of bytes sent, or -1 on error.
 */
ssize_t client_socket_sendall(struct client_socket *socket, const void *buf,
                              size_t len);

/**
 * Returns a string version of the address of this client.
 */
char* client_socket_addr_str(struct client_socket *socket);
