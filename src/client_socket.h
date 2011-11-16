/**
 * xmp3 - XMPP Proxy
 * client_socket.{c,h} - Abstracts basic socket interactions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <openssl/ssl.h>

// Forward declaration
struct client_socket;

/**
 * Function to call to send data to this socket.
 *
 * @param socket A client_socket structure to send to.
 * @param buf    The data to send.
 * @param len    The length of buf.
 * @return The number of bytes written, or <= 0 on error.
 */
typedef ssize_t (*socket_send_func)(struct client_socket *socket,
                                    const void *buf, size_t len);

/**
 * Function to call to receive data from this socket.
 *
 * @param socket A client_socket structure to receive from.
 * @param buf    A buffer to write data into
 * @param len    The length of buf.
 * @return The number of bytes written, <= 0 on error.
 */
typedef ssize_t (*socket_recv_func)(struct client_socket *socket,
                                    void *buf, size_t len);

/**
 * Function to call to close a socket.
 *
 * @param socket The client_socket to close.
 */
typedef void (*socket_close_func)(struct client_socket *socket);

/**
 * Function to call to clean up and delete the client_socket.
 *
 * This should clean up anything that was put in the client_socket->data item.
 * This should NOT close the socket.
 *
 * @param socket The client_socket to delete.
 */
typedef void (*socket_del_func)(struct client_socket *socket);

/** An abstract representation of a socket. */
struct client_socket {
    /** Function used for sending data. */
    socket_send_func send_func;

    /** Function used for receiving data. */
    socket_recv_func recv_func;

    /** Function used to close the socket. */
    socket_close_func close_func;

    /** Function used to delete the "data" pointer. */
    socket_del_func del_func;

    /** Implementation-specific data structure. */
    void *data;
};

/**
 * Allocates and initializes a new client socket using a file descriptor.
 *
 * Reading/writing is done just with send/recv.
 *
 * @param fd The file descriptor to read/write from.
 * @return A client_socket structure.
 */
struct client_socket* client_socket_new(int fd);

/**
 * Allocates and initializes a new client socket using SSL.
 *
 * @param ssl_context An OpenSSL context.
 * @param fd A connected file descriptor.
 * @return A client_socket structure.
 */
struct client_socket* client_socket_ssl_new(
        SSL_CTX *ssl_context, struct client_socket *orig_socket);

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
 * Closes a client_socket.
 *
 * @param socket The client socket to close.
 */
void client_socket_close(struct client_socket *socket);

/**
 * Cleans up and deallocates a client_socket structure.
 *
 * Note, this does NOT try to close the socket beforehand.
 *
 * @param socket The client_socket to delete.
 */
void client_socket_del(struct client_socket *socket);
