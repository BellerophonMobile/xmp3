/**
 * xmp3 - XMPP Proxy
 * xmp3_options.{c,h} - Functions for maintaining settings for XMP3
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include "xmp3_options.h"

#include <stdlib.h>
#include <errno.h>

#include "log.h"

// Default address is loopback
const struct in_addr DEFAULT_ADDR = { 0x0100007f };
const uint16_t DEFAULT_PORT = 5222;
const int DEFAULT_BACKLOG = 3;
const size_t DEFAULT_BUFFER_SIZE = 2000;
extern const bool DEFAULT_USE_SSL = true;
const char *DEFAULT_KEYFILE = "server.pem";
const char *DEFAULT_CERTFILE = "server.crt";
const char *DEFAULT_SERVER_NAME = "proxy";

/** Hold all the options used to configure the XMP3 server. */
struct xmp3_options {
    struct in_addr addr;
    uint16_t port;

    int backlog;
    size_t buffer_size;

    bool use_ssl;
    char *keyfile;
    char *certfile;

    char *server_name;
};

// Forward declarations
static bool read_int(const char *arg, long int *output);
static bool copy_string(char *dest, const char *src);

struct xmp3_options* xmp3_options_new() {
    struct xmp3_options *options = calloc(1, sizeof(*options));
    check_mem(options);

    options->addr = DEFAULT_ADDR;
    options->port = DEFAULT_PORT;
    options->backlog = DEFAULT_BACKLOG;
    options->buffer_size = DEFAULT_BUFFER_SIZE;

    options->use_ssl = DEFAULT_USE_SSL;
    options->keyfile = strdup(DEFAULT_KEYFILE);
    check_mem(options->keyfile);

    options->certfile = strdup(DEFAULT_CERTFILE);
    check_mem(options->certfile);

    options->server_name = strdup(DEFAULT_SERVER_NAME);
    check_mem(options->server_name);

    return options;
}

void xmp3_options_del(struct xmp3_options *options) {
    free(options->keyfile);
    free(options->certfile);
    free(options->server_name);
    free(options);
}

bool xmp3_options_set_addr_str(struct xmp3_options *options, const char *addr)
{
    return inet_aton(addr, &options->addr) != 0;
}

struct in_addr xmp3_options_get_addr(const struct xmp3_options *options) {
    return options->addr;
}

bool xmp3_options_set_port(struct xmp3_options *options, uint16_t port) {
    options->port = port;
    return true;
}

bool xmp3_options_set_port_str(struct xmp3_options *options, const char *str) {
    long int port;
    if (!read_int(str, &port)) {
        return false;
    }

    if (port > 65535 || port < 0) {
        return false;
    }

    options->port = port;
    return true;
}

uint16_t xmp3_options_get_port(const struct xmp3_options *options) {
    return options->port;
}

bool xmp3_options_set_backlog(struct xmp3_options *options, int backlog) {
    options->backlog = backlog;
    return true;
}

int xmp3_options_get_backlog(const struct xmp3_options *options) {
    return options->backlog;
}

bool xmp3_options_set_buffer_size(struct xmp3_options *options, size_t size) {
    options->buffer_size = size;
    return true;
}

size_t xmp3_options_get_buffer_size(const struct xmp3_options *options) {
    return options->buffer_size;
}

bool xmp3_options_set_ssl(struct xmp3_options *options, bool use_ssl) {
    options->use_ssl = use_ssl;
    return true;
}

bool xmp3_options_get_ssl(const struct xmp3_options *options) {
    return options->use_ssl;
}

bool xmp3_options_set_keyfile(struct xmp3_options *options,
                              const char *keyfile) {
    return copy_string(options->keyfile, keyfile);
}

const char* xmp3_options_get_keyfile(const struct xmp3_options *options) {
    return options->keyfile;
}

bool xmp3_options_set_certificate(struct xmp3_options *options,
                                  const char *certfile) {
    return copy_string(options->certfile, certfile);
}

const char* xmp3_options_get_certificate(const struct xmp3_options *options) {
    return options->certfile;
}

bool xmp3_options_set_server_name(struct xmp3_options *options,
                                  const char *name) {
    return copy_string(options->server_name, name);
}

const char* xmp3_options_get_server_name(const struct xmp3_options *options) {
    return options->server_name;
}

/**
 * Converts a string to an integer with error checking.
 *
 * @param[in]  arg    String value to convert.
 * @param[out] output Where to write the converted integer.
 * @return true if string is valid and conversion is successful, false
 *              otherwise.
 */
static bool read_int(const char *arg, long int *output) {
    // Clear errno so we can check if strtol fails.
    errno = 0;

    char *endptr;
    *output = strtol(arg, &endptr, 10);

    // We want to make sure we read the whole string.
    return (errno != ERANGE) && (*endptr == '\0');
}

static bool copy_string(char *dest, const char *src) {
    if (dest != NULL) {
        free(dest);
    }
    dest = strdup(src);
    check_mem(dest);
    return true;
}
