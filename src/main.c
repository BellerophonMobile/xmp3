/**
 * xmp3 - XMPP Proxy
 * main.c - Main function
 * Copyright (c) 2011 Drexel University
 * @file
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>

#include <expat.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "log.h"

#include "event.h"
#include "jid.h"
#include "xmp3_options.h"
#include "xmpp_client.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

static struct option long_options[] = {
    {"config",   required_argument, NULL, 'f'},
    {"addr",     required_argument, NULL, 'a'},
    {"port",     required_argument, NULL, 'p'},
    {"no-ssl",   no_argument,       NULL, 'n'},
    {"ssl-key",  required_argument, NULL, 'k'},
    {"ssl-cert", required_argument, NULL, 'c'},
    {"help",     no_argument,       NULL, 'h'},
    { NULL, 0, NULL, 0},
};

static void print_usage() {
    printf("./xmp3 [OPTIONS]\nOptions:\n");

    printf("  -f, --config   Config file to load.  Arguments override values"
           " in config file.\n");
    printf("  -a, --addr     Address to listen for incoming XMPP"
           " client connections (Default: %s)\n",
           inet_ntoa(DEFAULT_ADDR));
    printf("  -p, --port     Port to listen for incoming XMPP client"
           " connections (Default: %d)\n", DEFAULT_PORT);
    printf("  -n, --no-ssl   Disable SSL connection support\n");
    printf("  -k, --ssl-key  Path to the SSL private key to use"
           " (Default: %s)\n", DEFAULT_KEYFILE);
    printf("  -c, --ssl-cert Path to the SSL certificate to use"
           " (Default: %s)\n", DEFAULT_CERTFILE);
    printf("  -h, --help     This help output\n");
}

// File-global event loop handle, so we can stop it from the signal handler.
static struct event_loop *loop = NULL;

static void signal_handler(int signal) {
    if (signal == SIGINT) {
        event_loop_stop(loop);
    }
}

int main(int argc, char *argv[]) {
    char *conffile = NULL;
    char *address = NULL;
    char *port = NULL;
    bool ssl = true;
    char *keyfile = NULL;
    char *certfile = NULL;

    int c = 0;
    while (true) {
        c = getopt_long(argc, argv, "f:a:p:k:nc:h", long_options, NULL);

        if (c < 0) {
            break;
        }

        switch (c) {
            case 'f':
                conffile = optarg;
                break;
            case 'a':
                address = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'n':
                ssl = false;
                break;
            case 'k':
                keyfile = optarg;
                break;
            case 'c':
                certfile = optarg;
                break;
            case 'h':
                print_usage();
                return EXIT_SUCCESS;
            default:
                log_err("Invaid option");
                print_usage();
                return EXIT_FAILURE;
        }
    }

    struct xmp3_options *options = xmp3_options_new();
    check_mem(options);

    if (conffile) {
        check(xmp3_options_load_conf_file(options, conffile),
              "Error loading configuration file \"%s\"", conffile);
    }
    if (address) {
        check(xmp3_options_set_addr_str(options, address),
              "Invalid client address \"%s\"", address);
    }
    if (port) {
        check(xmp3_options_set_port_str(options, port),
              "Invalid client port \"%s\"", port);
    }
    if (!ssl) {
        check(xmp3_options_set_ssl(options, false),
              "Failed to disable openssl.");
    }
    if (keyfile) {
        check(xmp3_options_set_keyfile(options, keyfile),
              "Invalid keyfile \"%s\"", keyfile);
    }
    if (certfile) {
        check(xmp3_options_set_certificate(options, certfile),
              "Invalid certificate \"%s\"", certfile);
    }

    printf("Starting xmp3...\n");

    struct sigaction sa = {
        .sa_handler = signal_handler,
        .sa_flags = 0,
    };

    check(sigaction(SIGINT, &sa, NULL) != -1, "Cannot set signal handler.");

    loop = event_new_loop();

    if (xmp3_options_get_ssl(options)) {
        // Initialize OpenSSL
        SSL_load_error_strings();
        SSL_library_init();
    }

    struct xmpp_server *server = xmpp_server_new(loop, options);
    check(server != NULL, "XMPP server initialization failed");

    log_info("Starting event loop...");
    event_loop_start(loop);
    log_info("Event loop exited");

    if (xmp3_options_get_ssl(options)) {
        ERR_free_strings();
    }

    xmpp_server_del(server);
    event_del_loop(loop);
    xmp3_options_del(options);

    return EXIT_SUCCESS;

error:
    if (loop) {
        event_del_loop(loop);
    }
    return EXIT_FAILURE;
}
