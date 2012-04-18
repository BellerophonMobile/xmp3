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
 * @file xmp3_multicast.c
 * Simple extension module that multicasts messages to other XMP3 instances.
 */

/**
 * @page modules Building XMPP Modules
 *
 * Defining the Module
 * -------------------
 *
 *  Each external module requires a global module definition.  Do this by
 *  defining a symbol named "XMP3_MODULE" of type struct xmp3_module.
 *  This struct defines function pointers to be called to instantiate,
 *  destroy, start, stop, and configure an instance of your module.
 *
 *  @see struct xmp3_module for documentation on each function a module should
 *  define.
 *
 * Instantiating the Module
 * ------------------------
 *
 *  Modules that will inject XMPP stanzas into the server need to construct
 *  xmpp_stanza structures to hand to the server.  The xmpp_parser object is
 *  created for this purpose.  Most modules will want to create in instance of
 *  this in their xmp3_module.mod_new functions, setting the stanza handler to
 *  be something simple like the remote_stanza_handler() in xmp3_multicast.c.
 *  The stanza handler then just forwards the stanza to the server stanza
 *  router.
 *
 *  @see remote_stanza_handler() in xmp3_multicast.c for an example.
 *
 * Configuring the Module
 * ----------------------
 *
 *  The xmp3_module.mod_conf function will be called for each (key = value)
 *  pair defined in the configuration file.
 *
 *  @see multicast_conf() fin xmp3_multicast.c for an example.
 *
 * Starting the Module
 * -------------------
 *
 *  The xmp3_module.mod_start function will be called for each module instance
 *  just before the server starts, after all the configuration options have
 *  been parsed.  Here, a module that wants to receive stanzas sent to the
 *  server can register a stanza route (using xmpp_server_add_stanza_route()).
 *  Stanza routes look like normal JIDs, which are of the form
 *  "local@domain/resource".  Any part of the route can be replaced by an
 *  asterisk ("*") as a wildcard.
 *
 *  If a module creates and listens on its own sockets, it can utilize XMP3's
 *  event loop, instead of implementing its own in a thread.  Using the
 *  xmpp_server_loop() function to retrieve the instance of the main event
 *  loop, a module can then register its own callbacks with sockets with the
 *  event_register_callback() function.
 *
 *  @see multicast_start() in xmp3_multicast.c for an example.
 *
 * Stopping the Module
 * -------------------
 *
 *  The xmp3_module.mod_stop function will be called just after the server
 *  stops.  Here, the module should deregister any stanza routes it used (with
 *  xmp3_server_del_stanza_route()), and remove any event loop callbacks (with
 *  event_deregister_callback()).
 *
 *  @see multicast_stop() in xmp3_multicast.c for an example.
 *
 * Getting Stanzas from the Server
 * -------------------------------
 *
 *  By setting up a stanza route, a module will receive an xmpp_stanza instance
 *  for each stanza with a "to" attribute that matches the route (after
 *  evaluating wildcards).  In most cases, the stanza should not be modified,
 *  since other functions may still need to use that stanza instance.  From
 *  here, a module can serialize the stanza into any desired format and
 *  transmitted to another system.
 *
 *  @note If the module also injects stanzas into the server, the module will
 *  receive those stanzas if they match the stanza route added.  A work around
 *  for this is to check if a stanza is from a locally connected client (using
 *  xmpp_server_find_client()) before doing anything with it.
 *
 *  @see local_stanza_handler() in xmp3_multicast.c for an example.
 *
 * Injecting Stanzas to the Server
 * -------------------------------
 *
 *  If a module uses the xmpp_parser functions, it can pass received XMPP
 *  stanzas into the xmpp_parer_parse() function to construct an xmpp_stanza
 *  instance.  This stanza can then be passed to the xmpp_server_route_stanza()
 *  function to have the server process it.
 *
 *  @see socket_handler() and remote_stanza_handler() in xmp3_multicast.c for
 *  and example.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "event.h"
#include "jid.h"
#include "log.h"
#include "utils.h"
#include "xmp3_module.h"
#include "xmpp_parser.h"
#include "xmpp_server.h"
#include "xmpp_stanza.h"

/* Forward declarations. */
struct xmp3_multicast;

static void* multicast_new(void);
static void multicast_del(void *data);
static bool multicast_conf(void *data, const char *key, const char *value);
static bool multicast_start(void *data, struct xmpp_server *server);
static bool multicast_stop(void *data);

static bool local_stanza_handler(struct xmpp_stanza *stanza,
                                 struct xmpp_server *server, void *data);

static void socket_handler(struct event_loop *loop, int fd, void *data);
static bool remote_stanza_handler(struct xmpp_stanza *stanza,
                                  struct xmpp_parser *parser, void *data);

static bool bind_socket(struct xmp3_multicast *mcast);

/* Global module definition. */
struct xmp3_module XMP3_MODULE = {
    .mod_new = multicast_new,
    .mod_del = multicast_del,
    .mod_conf = multicast_conf,
    .mod_start = multicast_start,
    .mod_stop = multicast_stop,
};

/* Default parameters. */
static const char *DEFAULT_ADDRESS = "225.1.2.104";
static const uint16_t DEFAULT_PORT = 6010;
static const int DEFAULT_TTL = 64;
static const size_t DEFAULT_BUFFER_SIZE = 30720;

/** Structure managing the state of this module. */
struct xmp3_multicast {
    /** Address to listen on. */
    char *address;

    /** Port to listen on. */
    uint16_t port;

    /** Multicast TTL to set. */
    int ttl;

    /** The size of the receive buffer. */
    size_t buffer_size;

    /** The socket file descriptor. */
    int sock;

    /** The address to send to. */
    struct sockaddr_in send_addr;

    /** The receive buffer. */
    char *buffer;

    /** A reference to the XMPP server. */
    struct xmpp_server *server;

    /** The parser used for data received over the multicast socket. */
    struct xmpp_parser *parser;
};

static void* multicast_new(void) {
    struct xmp3_multicast *mcast = calloc(1, sizeof(*mcast));
    check_mem(mcast);

    /* Set default parameters. */
    STRDUP_CHECK(mcast->address, DEFAULT_ADDRESS);
    mcast->port = DEFAULT_PORT;
    mcast->ttl = DEFAULT_TTL;
    mcast->buffer_size = DEFAULT_BUFFER_SIZE;

    /* Create an XMPP parser for ourselves, so we can inject stanza object into
     * the server. */
    mcast->parser = xmpp_parser_new(false);
    xmpp_parser_set_handler(mcast->parser, remote_stanza_handler);
    xmpp_parser_set_data(mcast->parser, mcast);

    return mcast;
}

static void multicast_del(void *data) {
    struct xmp3_multicast *mcast = data;
    xmpp_parser_del(mcast->parser);
    free(mcast->address);
    free(data);
}

static bool multicast_conf(void *data, const char *key, const char *value) {
    struct xmp3_multicast *mcast = data;

    /* Compare the key against all the configuration options this module
     * accepts.  If it doesn't match any of them, return an error. */
    if (strcmp(key, "address") == 0) {
        copy_string(&mcast->address, value);

    } else if (strcmp(key, "port") == 0) {
        long int port;
        if (!read_int(value, &port)) {
            return false;
        }
        /* Check that port number is within bounds. */
        if (port > 65535 || port < 0) {
            return false;
        }
        mcast->port = port;

    } else if (strcmp(key, "bufsize") == 0) {
        long int size;
        return read_int(value, &size);
        mcast->buffer_size = size;

    } else {
        log_err("No such configuration option: '%s'", key);
        return false;
    }

    return true;
}

static bool multicast_start(void *data, struct xmpp_server *server) {
    struct xmp3_multicast *mcast = data;
    mcast->server = server;

    if (!bind_socket(mcast)) {
        goto error;
    }

    /* Add a callback to XMP3 so we can grab all messages created by a locally
     * connected client. */
    struct jid *jid = jid_new_from_str("*@*/*");
    xmpp_server_add_stanza_route(server, jid, local_stanza_handler, mcast);
    jid_del(jid);

    /* Allocate our receive buffer. */
    mcast->buffer = malloc(mcast->buffer_size * sizeof(char));
    check_mem(mcast->buffer);

    /* Add a callback to the event loop so we can get messages over our
     * multicast socket. */
    event_register_callback(xmpp_server_loop(server), mcast->sock,
                            socket_handler, mcast);

    return true;
error:
    return false;
}

static bool multicast_stop(void *data) {
    struct xmp3_multicast *mcast = data;

    struct jid *jid = jid_new_from_str("*@*/*");
    xmpp_server_del_stanza_route(mcast->server, jid, local_stanza_handler,
                                 mcast);
    jid_del(jid);

    event_deregister_callback(xmpp_server_loop(mcast->server), mcast->sock);

    /* Currently, the event loop closes all sockets when the event loop exits.
     * This should probably be changed.  When it does, we will need to clean
     * up our own socket. */
#if 0
    struct ip_mreq req = {
        .imr_multiaddr = { inet_addr(mcast->address) },
        .imr_interface.s_addr = htonl(INADDR_ANY),
    };
    if (setsockopt(mcast->sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &req,
                   sizeof(req)) != 0) {
        log_err("Cannot leave multicast group.");
    }

    if (shutdown(mcast->sock, SHUT_RDWR) == -1) {
        log_err("Unable to shut down multicast socket.");
    }

    if (close(mcast->sock) == -1) {
        log_err("Unable to close multicast socket");
    }
#endif

    if (mcast->buffer != NULL) {
        free(mcast->buffer);
    }
    return true;
}

static bool local_stanza_handler(struct xmpp_stanza *stanza,
                                 struct xmpp_server *server, void *data) {
    struct xmp3_multicast *mcast = data;

    if (strcmp(xmpp_stanza_name(stanza), XMPP_STANZA_IQ) == 0) {
        debug("Ignoring IQ stanza.");
        return true;
    }

    /* We only want to send out stanzas originating from locally connected
     * clients.  Look up the JID of the stanza's "from" attribute to see if
     * that JID is connected to this local server instance. */
    struct jid *from_jid = jid_new_from_str(xmpp_stanza_attr(stanza,
                XMPP_STANZA_ATTR_FROM));

    struct xmpp_client *client = xmpp_server_find_client(mcast->server,
                                                         from_jid);
    jid_del(from_jid);
    if (client == NULL) {
        debug("Ignoring stanza from non-local client.");
        return true;
    }

    size_t stanza_length;
    char *stanza_data = xmpp_stanza_string(stanza, &stanza_length);

    ssize_t num_sent = sendto(mcast->sock, stanza_data, stanza_length, 0,
                              (struct sockaddr*)&mcast->send_addr,
                              sizeof(mcast->send_addr));
    free(stanza_data);
    check(num_sent > 0, "Failed to send data on multicast socket.");
    check((size_t)num_sent == stanza_length,
          "Sent short message: %zd/%zd bytes", num_sent, stanza_length);
    log_info("Sent %zd bytes to multicast.", num_sent);
    return true;

error:
    return false;
}

static void socket_handler(struct event_loop *loop, int fd, void *data) {
    struct xmp3_multicast *mcast = data;

    struct sockaddr_in recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    ssize_t num_recv = recvfrom(fd, mcast->buffer, mcast->buffer_size, 0,
                                (struct sockaddr*)&recv_addr,
                                &recv_addr_len);
    check(num_recv > 0, "Failed to receive from multicast socket.");

    log_info("Received %zd bytes from %s:%d", num_recv,
             inet_ntoa(recv_addr.sin_addr), ntohs(recv_addr.sin_port));

    /* Parse the XMPP stanza string we just received.  Reset the parser before
     * to clear any state from previous stanzas. */
    xmpp_parser_reset(mcast->parser, false);
    xmpp_parser_parse(mcast->parser, mcast->buffer, num_recv);

error:
    return;
}

static bool remote_stanza_handler(struct xmpp_stanza *stanza,
                                  struct xmpp_parser *parser, void *data) {
    struct xmp3_multicast *mcast = data;
    return xmpp_server_route_stanza(mcast->server, stanza);
}

static bool bind_socket(struct xmp3_multicast *mcast) {
    check((mcast->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) >= 0,
          "Cannot create multicast socket.");

    int flag = 0;
    check(setsockopt(mcast->sock, IPPROTO_IP, IP_MULTICAST_LOOP, &flag,
                     sizeof(flag)) == 0,
          "Cannot disable multicast loopback.");

    check(setsockopt(mcast->sock, IPPROTO_IP, IP_MULTICAST_TTL, &mcast->ttl,
                     sizeof(mcast->ttl)) == 0,
          "Cannot set multicast TTL.");

    flag = 1;
    check(setsockopt(mcast->sock, SOL_SOCKET, SO_REUSEADDR, &flag,
                     sizeof(flag)) == 0,
          "Cannot make socket reusable.");

    struct sockaddr_in sa = {
        .sin_family = AF_INET,
        .sin_port = htons(mcast->port),
        .sin_addr = { htonl(INADDR_ANY) },
    };
    check(bind(mcast->sock, (struct sockaddr*)&sa, sizeof(sa)) == 0,
          "Cannot bind multicast socket.");

    struct ip_mreq req = {
        .imr_multiaddr.s_addr = inet_addr(mcast->address),
        .imr_interface.s_addr = htonl(INADDR_ANY),
    };
    check(setsockopt(mcast->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                     &req, sizeof(req)) == 0,
          "Cannot join multicast group.");

    memset(&mcast->send_addr, 0, sizeof(mcast->send_addr));
    mcast->send_addr.sin_family = AF_INET;
    mcast->send_addr.sin_addr.s_addr = inet_addr(mcast->address);
    mcast->send_addr.sin_port = htons(mcast->port);

    log_info("Joined multicast group %s:%d", mcast->address, mcast->port);

    return true;
error:
    return false;
}
