/**
 * xmp3 - XMPP Proxy
 * main.c - Main function
 * Copyright (c) 2011 Drexel University
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>

#include "event.h"
#include "xmpp.h"

static const struct in_addr DEFAULT_CLIENT_ADDR = { 0x0100007f };
static const uint16_t DEFAULT_CLIENT_PORT = 5222;

static struct option long_options[] = {
    {"client-addr", required_argument, NULL, 'a'},
    {"client-port", required_argument, NULL, 'p'},
    {"help",        no_argument,       NULL, 'h'},
    NULL,
};

static void print_usage() {
    printf("./xmp3 [OPTIONS]\nOptions:\n");

    printf("  -a, --client-addr     Address to listen for incoming XMPP\n"
           "                        client connections (Default: %s)\n",
           inet_ntoa(DEFAULT_CLIENT_ADDR));
    printf("  -p, --client-port     Port to listen for incoming XMPP client\n"
           "                        connections (Default: %d)\n",
           DEFAULT_CLIENT_PORT);
    printf("  -h, --help            This help output\n");
}

static bool read_port(const char *arg, uint16_t *output) {
    char *endptr;
    size_t len = strnlen(arg, 5);

    errno = 0;
    *output = strtol(optarg, &endptr, 10);
    return (errno != ERANGE) && (endptr == optarg + len);
}

int main(int argc, char *argv[]) {
    struct in_addr client_addr = DEFAULT_CLIENT_ADDR;
    uint16_t client_port = DEFAULT_CLIENT_PORT;

    int c = 0;
    while (true) {
        c = getopt_long(argc, argv, "a:p:h", long_options, NULL);

        if (c < 0) {
            break;
        }

        switch (c) {
            case 'a':
                if (!inet_aton(optarg, &client_addr)) {
                    fprintf(stderr, "Invalid client address \"%s\"\n", optarg);
                    return EXIT_FAILURE;
                }
                break;

            case 'p':
                if (!read_port(optarg, &client_port)) {
                    fprintf(stderr, "Invalid client port \"%s\"\n", optarg);
                    return EXIT_FAILURE;
                }
                break;

            case 'h':
                print_usage();
                return EXIT_SUCCESS;

            default:
                fprintf(stderr, "Invaid option\n");
                print_usage();
                return EXIT_FAILURE;
        }
    }

    printf("Starting xmp3...\n");

    struct event_loop *loop = event_new_loop();

    if (!xmpp_init(loop, client_addr, client_port)) {
        fprintf(stderr, "XMPP server initialization failed\n");
        return EXIT_FAILURE;
    }

    printf("Starting event loop...\n");
    event_loop_start(loop);

    event_del_loop(loop);

    return EXIT_SUCCESS;
}
