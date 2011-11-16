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
extern const char *DEFAULT_KEYFILE;
extern const char *DEFAULT_CERTFILE;

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
