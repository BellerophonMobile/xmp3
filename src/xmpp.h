/**
 * xmp3 - XMPP Proxy
 * xmpp.{c,h} - Implements the server part of a normal XMPP server.
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "event.h"
#include "xmp3_options.h"
#include "xmpp_common.h"

/**
 * Callback to deliver a message stanza.
 *
 * @param from_stanza The message stanza to be delivered.
 * @param data        Data from when this callback was registered.
 * @return True if message was handled, false if not.
 */
typedef bool (*xmpp_message_callback)(struct xmpp_stanza *from_stanza,
                                      void *data);

/**
 * Callback to deliver a presence stanza.
 *
 * @param from_stanza The presence stanza to be delivered.
 * @param data        Data from when this callback was registered.
 * @return True if message was handled, false if not.
 */
typedef bool (*xmpp_presence_callback)(struct xmpp_stanza *from_stanza,
                                       void *data);

/**
 * Callback to deliver a iq stanza.
 *
 * @param from_stanza The iq stanza to be delivered.
 * @param data        Data from when this callback was registered.
 * @return True if message was handled, false if not.
 */
typedef bool (*xmpp_iq_callback)(struct xmpp_stanza *from_stanza, void *data);

/** Opaque type for the XMPP server instance. */
struct xmpp_server;

/**
 * Initializes the XMPP server and listens for new connections.
 *
 * @param loop An event loop instance that the server can register callbacks.
 * @param options An XMP3 options instance to read for configuration.
 * @return A pointer to the XMPP server instance.
 */
struct xmpp_server* xmpp_init(struct event_loop *loop,
                              const struct xmp3_options *options);

/**
 * Cleans up and frees an XMPP server.
 *
 * @param server The XMPP server instance to shut down.
 */
void xmpp_shutdown(struct xmpp_server *server);

/**
 * Return a new OpenSSL SSL connection structure.
 *
 * Since the internals of the xmpp_server struct are hidden, we need this
 * function.
 *
 * @param client An XMPP client to switch to SSL
 */
void xmpp_new_ssl_connection(struct xmpp_client *client);

/**
 * Register a callback to deliver message stanzas.
 *
 * @param server An XMPP server instance.
 * @param jid    The JID to receive messages for.
 * @param cb     The callback to call when the server receives a message stanza
 *               for this JID.
 * @param data   Arbitrary data to be passed to the callback function.
 */
void xmpp_register_message_route(struct xmpp_server *server, struct jid *jid,
                                 xmpp_message_callback cb, void *data);

/**
 * Remove a callback for message stanzas.
 *
 * @param server An XMPP server instance.
 * @param jid    The JID to deregister callbacks for.
 */
void xmpp_deregister_message_route(struct xmpp_server *server,
                                   struct jid *jid);

/**
 * Deliver a message stanza to a registered callback function.
 *
 * @param stanza The XMPP stanza to route.
 * @return True if successfully handled, false if not.
 */
bool xmpp_route_message(struct xmpp_stanza *stanza);

/**
 * Register a callback to handle IQ stanza to a particular namespace+tag name.
 *
 * @param server An XMPP server instance.
 * @param ns     The namespace + name string to receive IQs for.
 * @param cb     The callback to call when the server receives a iq stanza for
 *               this namespace.
 * @param data   Arbitrary data to be passed to the callback function.
 */
void xmpp_register_iq_namespace(struct xmpp_server *server, const char *ns,
                                xmpp_iq_callback cb, void *data);

/**
 * Deregister a callback for IQ stanzas.
 *
 * @param server An XMPP server instance.
 * @param ns     The namespace + name to deregister callbacks for.
 */
void xmpp_deregister_iq_namespace(struct xmpp_server *server, const char *ns);

/**
 * Deliver an IQ stanza to a registered callback function.
 *
 * @param ns     The namespace + name string to deliver the stanza to.
 * @param stanza The stanza to deliver.
 * @return True if successfully handled, false if not.
 */
bool xmpp_route_iq(const char *ns, struct xmpp_stanza *stanza);
