/**
 * xmp3 - XMPP Proxy
 * utils.{c,h} - Miscellaneous utility functions.
 * Copyright (c) 2011 Drexel University
 */

#pragma once

#include "xmpp_common.h"

/**
 * Send all data in a buffer to the connected socket.
 *
 * This will keep trying until all data is sent.  This should later be
 * rewritten to add a write callback using a non-blocking socket.
 */
int sendall(int fd, const char *buffer, int len);

/** Allocates and returns a string version of a jid. */
char* jid_to_str(struct jid *jid);

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
