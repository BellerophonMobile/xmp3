/**
 * xmp3 - XMPP Proxy
 * utils.{c,h} - Miscellaneous utility functions.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <expat.h>

#include "client_socket.h"
#include "xmpp_common.h"

/**
 * Allocate and copy a string.
 *
 * This makes sure the final string is null-terminated.
 *
 * @param[out] dst Pointer to store the string in.
 * @param[in] src Source string to copy.
 */
#define STRDUP_CHECK(dst, src) do { \
    dst = strdup(src); \
    check_mem(dst); \
} while (0)

/**
 * Allocate and copy a string, up to size n.
 *
 * This makes sure the final string is null-terminated.
 *
 * @param[out] dst Pointer to store the string in.
 * @param[in] src  Source string to copy.
 * @param[in] n    Number of characters to copy.
 */
#define STRNDUP_CHECK(dst, src, n) do { \
    dst = strndup(src, n); \
    check_mem(dst); \
} while(0)

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
int sendall(struct client_socket *socket, const char *buffer, int len);

/**
 * Send the raw text of the current Expat parse event to a client.
 *
 * @param parser An Expat parser instance.
 * @param fd     The file descriptor to send to.
 */
int sendxml(XML_Parser parser, struct client_socket *socket);

/**
 * Takes a stanza struct and recreates the string tag.
 *
 * @param stanza Stanza structure to create a tag for.
 * @return A string representing the start stanza tag, with all attributes and
 *         their values.
 */
char* create_start_tag(struct xmpp_stanza *stanza);

/*
 * These functions are from libb64, found at: http://libb64.sourceforge.net/
 *
 * cdecode.h - c header for a base64 decoding algorithm
 * This is part of the libb64 project, and has been placed in the public
 * domain.  For details, see http://sourceforge.net/projects/libb64
 */

enum base64_decodestep {
    base64_step_a,
    base64_step_b,
    base64_step_c,
    base64_step_d,
};

struct base64_decodestate {
    enum base64_decodestep step;
    char plainchar;
};

void base64_init_decodestate(struct base64_decodestate *state_in);

int base64_decode_value(char value_in);

int base64_decode_block(const char *code_in, const int length_in,
                        char *plaintext_out,
                        struct base64_decodestate *state_in);
