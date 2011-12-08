/**
 * xmp3 - XMPP Proxy
 * client_socket.{c,h} - Abstracts basic socket interactions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>
#include <netinet/in.h>

#include <expat.h>
#include <openssl/ssl.h>

struct client_socket;

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

/** Closes, cleans up and deallocates a client_socket structure. */
void client_socket_del(struct client_socket *socket);

/** Closes a client_socket. */
void client_socket_close(struct client_socket *socket);

int client_socket_fd(const struct client_socket *socket);

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
 * Enables SSL on a client socket.
 *
 * @param socket The socket to toggle SSL on.
 * @param ssl_enabled True to enable SSL, false to disable it.
 * @returns True if successful, false if not.
 */
bool client_socket_set_ssl(struct client_socket *socket, SSL_CTX *ssl_context);

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
 * Send the raw text of the current Expat parse event to a client.
 *
 * @param parser An Expat parser instance.
 * @param fd     The file descriptor to send to.
 */
int client_socket_sendxml(struct client_socket *socket, XML_Parser parser);

/** Returns a newly allocated string version of the socket address. */
char* client_socket_addr_str(struct client_socket *socket);
