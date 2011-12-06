/**
 * xmp3 - XMPP Proxy
 * xmp3_options.{c,h} - Functions for maintaining settings for XMP3
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <arpa/inet.h>

extern const struct in_addr DEFAULT_ADDR;
extern const uint16_t DEFAULT_PORT;
extern const int DEFAULT_BACKLOG;
extern const size_t DEFAULT_BUFFER_SIZE;
extern const bool DEFAULT_USE_SSL;
extern const char *DEFAULT_KEYFILE;
extern const char *DEFAULT_CERTFILE;
extern const char *DEFAULT_SERVER_NAME;

/** Opaque pointer maintaining the options for XMP3. */
struct xmp3_options;

/** Allocate and initialize a new XMP3 options instance. */
struct xmp3_options* xmp3_options_new();

/** Cleans up and frees an XMP3 options instance. */
void xmp3_options_del(struct xmp3_options *options);

/**
 * Set the address that the server should bind to.
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_addr_str(struct xmp3_options *options, const char *addr);

/**
 * Get the address that the server will bind to.
 *
 * This just uses inet_ntoa, so subsequent calls will overwrite the buffer.
 *
 */
struct in_addr xmp3_options_get_addr(const struct xmp3_options *options);

/**
 * Set the port that the server should bind to.
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_port(struct xmp3_options *options, uint16_t port);

/**
 * Set the port that the server should bind to using a string.
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_port_str(struct xmp3_options *options, const char *port);

/** Get the port that the server will bind to. */
uint16_t xmp3_options_get_port(const struct xmp3_options *options);

/**
 * Set the amount of pending connections the server will support.
 *
 * This is the "backlog" parameter for the "listen()" function.
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_backlog(struct xmp3_options *options, int backlog);

/** Get the amount of pending connections the server will support. */
int xmp3_options_get_backlog(const struct xmp3_options *options);

/**
 * Set the size of the buffer used to receive incoming data.
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_buffer_size(struct xmp3_options *options, size_t size);

/** Get the size of the buffer used to receive incoming data. */
size_t xmp3_options_get_buffer_size(const struct xmp3_options *options);

/**
 * Enable/disable SSL support for the XMPP server.
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_ssl(struct xmp3_options *options, bool use_ssl);

/** Get whether SSL is enabled or disabled on the server. */
bool xmp3_options_get_ssl(const struct xmp3_options *options);

/** Set the path to the OpenSSL private key file to use. */
bool xmp3_options_set_keyfile(struct xmp3_options *options,
                              const char *keyfile);

/** Return the path to the OpenSSL private key file. */
const char* xmp3_options_get_keyfile(const struct xmp3_options *options);

/** Set the path to the OpenSSL certificate file. */
bool xmp3_options_set_certificate(struct xmp3_options *options,
                                  const char *certfile);

/** Get the path to the OpenSSL certificate file. */
const char* xmp3_options_get_certificate(const struct xmp3_options *options);

/**
 * Set the name of the XMPP server (used for the domainpart).
 *
 * @return true if successful, false if not.
 */
bool xmp3_options_set_server_name(struct xmp3_options *options,
                                  const char *name);

/** Gets the name of the XMPP server. */
const char* xmp3_options_get_server_name(const struct xmp3_options *options);
