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

#define MAX_PATH_LEN 256

// Default address is loopback
const struct in_addr DEFAULT_ADDR = { 0x0100007f };
const uint16_t DEFAULT_PORT = 5222;
const char *DEFAULT_KEYFILE = "server.pem";
const char *DEFAULT_CERTFILE = "server.crt";

/** Hold all the options used to configure the XMP3 server. */
struct xmp3_options {
    struct in_addr addr;
    uint16_t port;
    char keyfile[MAX_PATH_LEN];
    char certfile[MAX_PATH_LEN];
};

// Forward declarations
static bool read_int(const char *arg, long int *output);

struct xmp3_options* xmp3_options_new() {
    struct xmp3_options *options = calloc(1, sizeof(*options));
    check_mem(options);

    options->addr = DEFAULT_ADDR;
    options->port = DEFAULT_PORT;
    strcpy(options->keyfile, DEFAULT_KEYFILE);
    strcpy(options->certfile, DEFAULT_CERTFILE);

    return options;
}

void xmp3_options_del(struct xmp3_options *options) {
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

bool xmp3_options_set_keyfile(struct xmp3_options *options,
                              const char *keyfile) {
    if (strnlen(keyfile, MAX_PATH_LEN + 1) == MAX_PATH_LEN + 1) {
        return false;
    }
    strncpy(options->keyfile, keyfile, MAX_PATH_LEN - 1);
    options->keyfile[MAX_PATH_LEN - 1] = '\0';
    return true;
}

const char* xmp3_options_get_keyfile(const struct xmp3_options *options) {
    return options->keyfile;
}

bool xmp3_options_set_certificate(struct xmp3_options *options,
                                  const char *certfile) {
    if (strnlen(certfile, MAX_PATH_LEN + 1) == MAX_PATH_LEN + 1) {
        return false;
    }
    strncpy(options->certfile, certfile, MAX_PATH_LEN - 1);
    options->certfile[MAX_PATH_LEN - 1] = '\0';
    return true;
}

const char* xmp3_options_get_certificate(const struct xmp3_options *options) {
    return options->certfile;
}
