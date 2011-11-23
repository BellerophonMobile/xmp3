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
 * @param a Destination pointer.
 * @param b Source string.
 */
#define ALLOC_COPY_STRING(a, b) do { \
    a = calloc(strlen(b) + 1, sizeof(char)); \
    check_mem(a); \
    strcpy(a, b); \
} while (0)

/**
 * Allocate and push a string into an array.
 *
 * @param a Destination array.
 * @param b Source string.
 */
#define ALLOC_PUSH_BACK(a, b) do { \
    char *tmp = calloc(strlen(b), sizeof(*tmp)); \
    check_mem(tmp); \
    utarray_push_back(a, tmp); \
} while (0)


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
 * Allocates and returns a string version of a jid.
 *
 * @param jid JID structure to convert.
 * @return The JID converted to a string.
 */
char* jid_to_str(const struct jid *jid);

/**
 * Fills in a previously allocated jid structure from a string.
 *
 * @param[in] str String JID to convert.
 * @param[out] jid Empty (but allocated) JID structure to fill in.
 */
void str_to_jid(const char *str, struct jid *jid);

/**
 * Gets the string length of a jid.
 *
 * Doesn't allocate any memory, just gets the length of the JID struct if it
 * were a string.
 *
 * @param jid JID struct to find the length of.
 * @return The number of bytes used to contain the JID as a string.
 */
ssize_t jid_len(struct jid *jid);

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
