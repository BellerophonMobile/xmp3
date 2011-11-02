/**
 * xmp3 - XMPP Proxy
 * utils.{c,h} - Miscellaneous utility functions.
 * Copyright (c) 2011 Drexel University
 */

#include "utils.h"

#include <sys/types.h>
#include <sys/socket.h>

#include "utstring.h"

#include "log.h"

int sendall(int fd, const char *buffer, int len) {
    ssize_t numsent = 0;
    do {
        ssize_t newsent = send(fd, buffer + numsent, len - numsent, 0);
        check(newsent != -1, "Error sending data");
        numsent += newsent;
    } while (numsent < len);

    return numsent;
error:
    return -1;
}

int sendxml(XML_Parser parser, int fd) {
    int offset, size;
    const char *buffer = XML_GetInputContext(parser, &offset, &size);
    check_mem(buffer);
    int count = XML_GetCurrentByteCount(parser);

    return sendall(fd, buffer + offset, count);
}

char* jid_to_str(struct jid *jid) {
    UT_string strjid;
    utstring_init(&strjid);

    // Domain part is required
    if (jid->domain != NULL) {
        if (jid->local == NULL) {
            utstring_printf(&strjid, jid->domain);
        } else {
            utstring_printf(&strjid, "%s@%s", jid->local, jid->domain);
        }

        if (jid->resource != NULL) {
            utstring_printf(&strjid, "/%s", jid->resource);
        }
    }

    return utstring_body(&strjid);
}

void str_to_jid(const char *str, struct jid *jid) {
    char *at_delim = strchr(str, '@');
    int local_len = 0;
    if (at_delim != NULL) {
        local_len = at_delim - str;
        jid->local = calloc(local_len, sizeof(*jid->local));
        check_mem(jid->local);
        strncpy(jid->local, str, local_len);
        debug("Setting local to '%s'", jid->local);
    }

    char *slash_delim = strchr(str, '/');
    int resource_len = 0;
    if (slash_delim != NULL) {
        resource_len = strchr(str, '\0') - slash_delim;
        jid->resource = calloc(resource_len, sizeof(*jid->resource));
        check_mem(jid->resource);
        strncpy(jid->resource, slash_delim + 1, resource_len);
        debug("Setting resource to '%s'", jid->resource);
    }

    int domain_len = strlen(str) - local_len - resource_len;
    jid->domain = calloc(domain_len, sizeof(*jid->domain));
    check_mem(jid->domain);
    strncpy(jid->domain, str + local_len + 1, domain_len - 1);
    debug("Setting domain to '%s'", jid->domain);
}

ssize_t jid_len(struct jid *jid) {
    ssize_t len = 1; // For '@'
    if (jid->local != NULL) {
        len += strlen(jid->local);
    }
    if (jid->domain != NULL) {
        len += strlen(jid->domain);
    }
    if (jid->resource != NULL) {
        len += strlen(jid->resource) + 1; // +1 for '/'
    }
    return len;
}

char* create_start_tag(struct xmpp_stanza *stanza) {
    UT_string s;
    utstring_init(&s);

    utstring_printf(&s, "<%s", stanza->name);

    if (stanza->namespace != NULL) {
        utstring_printf(&s, " xmlns='%s'", stanza->namespace);
    }

    if (stanza->id != NULL) {
        utstring_printf(&s, " id='%s'", stanza->id);
    }

    if (stanza->type != NULL) {
        utstring_printf(&s, " type='%s'", stanza->type);
    }

    if (stanza->to != NULL) {
        utstring_printf(&s, " to='%s'", stanza->to);
    }

    if (stanza->from != NULL) {
        utstring_printf(&s, " from='%s'", stanza->from);
    }

    for (int i = 0; i < utarray_len(stanza->other_attrs); i += 2) {
        utstring_printf(&s, " %s='%s'",
                utarray_eltptr(stanza->other_attrs, i),
                utarray_eltptr(stanza->other_attrs, i + 1));
    }

    utstring_printf(&s, ">");

    return utstring_body(&s);
}

/*
 * These functions are from libb64, found at: http://libb64.sourceforge.net/
 *
 * cdecoder.c - c source to a base64 decoding algorithm implementation
 *
 * This is part of the libb64 project, and has been placed in the public
 * domain. For details, see http://sourceforge.net/projects/libb64
 */

void base64_init_decodestate(struct base64_decodestate *state_in) {
    state_in->step = base64_step_a;
    state_in->plainchar = 0;
}

int base64_decode_value(char value_in) {
    static const char decoding[] = {62, -1, -1, -1, 63, 52, 53, 54, 55, 56, 57,
        58, 59, 60, 61, -1, -1, -1, -2, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1,
        -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
    static const char decoding_size = sizeof(decoding);
    value_in -= 43;
    if (value_in < 0 || value_in > decoding_size) {
        return -1;
    }
    return decoding[(int)value_in];
}

int base64_decode_block(const char *code_in, const int length_in,
        char *plaintext_out,
        struct base64_decodestate *state_in) {
    const char *codechar = code_in;
    char *plainchar = plaintext_out;
    char fragment;

    *plainchar = state_in->plainchar;

    switch (state_in->step) {
        while (1) {
            case base64_step_a:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_a;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar = (fragment & 0x03f) << 2;
            case base64_step_b:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_b;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x030) >> 4;
                *plainchar = (fragment & 0x00f) << 4;
            case base64_step_c:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_c;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x03c) >> 2;
                *plainchar = (fragment & 0x003) << 6;
            case base64_step_d:
                do {
                    if (codechar == code_in + length_in) {
                        state_in->step = base64_step_d;
                        state_in->plainchar = *plainchar;
                        return plainchar - plaintext_out;
                    }
                    fragment = (char)base64_decode_value(*codechar++);
                } while (fragment < 0);
                *plainchar++ |= (fragment & 0x03f);
        }
    }
    /* control should not reach here */
    return plainchar - plaintext_out;
}
