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
 * @file xmp3_options.c
 * Functions for maintaining settings for XMP3
 */

#include <arpa/inet.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include <ini.h>
#include <tj_searchpathlist.h>

#include "log.h"
#include "utils.h"
#include "xmp3_module.h"
#include "xmp3_options.h"

/* Default address is loopback. */
const struct in_addr DEFAULT_ADDR = { 0x0100007f };
const uint16_t DEFAULT_PORT = 5222;
const int DEFAULT_BACKLOG = 3;
const size_t DEFAULT_BUFFER_SIZE = 2000;
const bool DEFAULT_USE_SSL = true;
const char *DEFAULT_KEYFILE = "server.pem";
const char *DEFAULT_CERTFILE = "server.crt";
const char *DEFAULT_SERVER_NAME = "localhost";

/** Hold all the options used to configure the XMP3 server. */
struct xmp3_options {
    /** Address to listen on. */
    struct in_addr addr;

    /** Port to listen on. */
    uint16_t port;

    /** Backlog of connections for the "listen" function. */
    int backlog;

    /** Size of the incoming message buffer. */
    size_t buffer_size;

    /** Whether or not to use SSL. */
    bool use_ssl;

    /** The path to the OpenSSL key file. */
    char *keyfile;

    /** The path to the OpenSSL certificate file. */
    char *certfile;

    /**
     * The name of the server (domainpart of JID).
     *
     * TODO: Not currently used anywhere.
     */
    char *server_name;

    /** List of directories to search for loadable modules. */
    tj_searchpathlist *search_path;

    /** Structure holding currently loaded modules. */
    struct xmp3_modules *modules;
};

// Forward declarations
static int ini_handler(void *data, const char *section, const char *name,
                        const char *value);

struct xmp3_options* xmp3_options_new(void) {
    struct xmp3_options *options = calloc(1, sizeof(*options));
    check_mem(options);

    /* Set default parameters. */
    options->addr = DEFAULT_ADDR;
    options->port = DEFAULT_PORT;
    options->backlog = DEFAULT_BACKLOG;
    options->buffer_size = DEFAULT_BUFFER_SIZE;

    options->use_ssl = DEFAULT_USE_SSL;

    STRDUP_CHECK(options->keyfile, DEFAULT_KEYFILE);
    STRDUP_CHECK(options->certfile, DEFAULT_CERTFILE);
    STRDUP_CHECK(options->server_name, DEFAULT_SERVER_NAME);

    options->search_path = tj_searchpathlist_create();
    check_mem(options->search_path);

    options->modules = xmp3_modules_new();
    check_mem(options->modules);

    return options;
}

void xmp3_options_del(struct xmp3_options *options) {
    free(options->keyfile);
    free(options->certfile);
    free(options->server_name);
    tj_searchpathlist_finalize(options->search_path);
    xmp3_modules_del(options->modules);
    free(options);
}

bool xmp3_options_load_conf_file(struct xmp3_options *options,
                                 const char *file) {
    return ini_parse(file, ini_handler, options) == 0;
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

    /* Check that port number is within bounds. */
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
    copy_string(&options->keyfile, keyfile);
    return true;
}

const char* xmp3_options_get_keyfile(const struct xmp3_options *options) {
    return options->keyfile;
}

bool xmp3_options_set_certificate(struct xmp3_options *options,
                                  const char *certfile) {
    copy_string(&options->certfile, certfile);
    return true;
}

const char* xmp3_options_get_certificate(const struct xmp3_options *options) {
    return options->certfile;
}

bool xmp3_options_set_server_name(struct xmp3_options *options,
                                  const char *name) {
    copy_string(&options->server_name, name);
    return true;
}

const char* xmp3_options_get_server_name(const struct xmp3_options *options) {
    return options->server_name;
}

bool xmp3_options_add_module_path(struct xmp3_options *options,
                                  const char *path) {
    /* Calculate the absolute path before adding it to the list. */
    char full_path[PATH_MAX];
    check(realpath(path, full_path) != NULL,
          "Unable to determine absolute path for: '%s'", path);
    check(tj_searchpathlist_add(options->search_path, full_path),
          "Unable to add '%s' to search path", full_path);
    return true;

error:
    return false;
}

struct xmp3_modules* xmp3_options_get_modules(struct xmp3_options *options) {
    return options->modules;
}

/** Callback used by config file parser. */
static int ini_handler(void *data, const char *section, const char *name,
                        const char *value) {
    struct xmp3_options *options = data;

    /* The top of the file (no section) is for configuring core XMP3. */
    if (strcmp(section, "") == 0) {
        // Main XMP3 options
        if (strcmp(name, "address") == 0) {
            return xmp3_options_set_addr_str(options, value);
        }

        if (strcmp(name, "port") == 0) {
            return xmp3_options_set_port_str(options, value);
        }

        if (strcmp(name, "ssl") == 0) {
            if (strcmp(value, "true") == 0) {
                return xmp3_options_set_ssl(options, true);
            } else if (strcmp(value, "false") == 0) {
                return xmp3_options_set_ssl(options, false);
            } else {
                log_err("Invalid value for ssl option: '%s'", value);
                return false;
            }
        }

        if (strcmp(name, "keyfile") == 0) {
            return xmp3_options_set_keyfile(options, value);
        }

        if (strcmp(name, "certificate") == 0) {
            return xmp3_options_set_certificate(options, value);
        }

        if (strcmp(name, "name") == 0) {
            return xmp3_options_set_server_name(options, value);
        }

        if (strcmp(name, "modpath") == 0) {
            return xmp3_options_add_module_path(options, value);
        }

        log_err("Unknown config item '%s = %s'", name, value);
        return false;

    /* The "modules" section is for loading modules.  Modules get assigned
     * to "name".  This name can then be used as sections to specify options
     * to the module. */
    } else if (strcmp(section, "modules") == 0) {
        char path[PATH_MAX];
        tj_searchpathlist_locate(options->search_path, value, path, PATH_MAX);
        if (!xmp3_modules_load(options->modules, path, name)) {
            log_err("Error loading module '%s' (%s)", path, value);
            return false;
        }

    /* Any other section is used to configure a module loaded in the "modules"
     * section. */
    } else {
        if (!xmp3_modules_config(options->modules, section, name, value)) {
            log_err("Error configuring module '%s'", section);
            return false;
        }
    }
    return true;
}
