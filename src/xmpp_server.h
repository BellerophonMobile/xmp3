/**
 * xmp3 - XMPP Proxy
 * server.{c,h} - Main XMPP server data/functions
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

#include "xmp3_options.h"
#include "jid.h"
#include "xmpp_client.h"
#include "xmpp_stanza.h"

struct xmpp_server;

/**
 * Callback to deliver an XMPP stanza.
 *
 * @param stanza The message stanza to be delivered.
 * @param data        Data from when this callback was registered.
 * @return True if message was handled, false if not.
 */
typedef bool (*xmpp_server_stanza_callback)(struct xmpp_stanza *stanza,
                                            void *data);

/**
 * Allocates and initializes the XMPP server.
 *
 * @param loop An event loop instance that the server can register callbacks.
 * @param options An XMP3 options instance to read for configuration.
 * @return A pointer to the XMPP server instance.
 */
struct xmpp_server* xmpp_server_new(struct event_loop *loop,
                                    const struct xmp3_options *options);

/**
 * Cleans up an XMPP server instance.
 *
 * @param server The server instance to destroy.
 */
void xmpp_server_del(struct xmpp_server *server);

/**
 * Causes an XMPP server instance to begin accepting new connections.
 *
 * @param server The server instance to start.
 * @return true if successful, false on error.
 */
bool xmpp_server_start(struct xmpp_server *server);

/**
 * Shuts down a running XMPP server instance.
 *
 * This will cleanly disconnect all connected clients, etc.
 *
 * @param server The server instance to stop.
 * @return true if successful, false on error.
 */
bool xmpp_server_stop(struct xmpp_server *server);

/**
 * Add a callback to the list of routes for stanza delivery.
 *
 * You can use wildcards ("*") to be notified of stanzas for a range of JIDs.
 * For example *@conference.localhost would receive all stanzas for
 * a conference server.  Also routes for a bare JID (no resource) will receive
 * stanzas addressed to all resources of that JID.
 *
 * Multiple routes matching the same JID can be added.  They will be called
 * in the order they are registered.
 *
 * TODO: Allow inserting callbacks in specific positions?
 *
 * Also, the JID passed in will be copied.  You retain ownership and can free
 * it anytime.
 *
 * @param server An XMPP server instance.
 * @param jid    The JID to receive stanzas for.  "*" strings serve as
 *               wildcards for any field.  This will be copied, so you can do
 *               anything with it.
 * @param cb     The callback to call when the server receives a stanza for
 *               this JID.
 * @param data   Arbitrary data to be passed to the callback function.
 */
void xmpp_server_add_stanza_route(struct xmpp_server *server,
                                  const struct jid *jid,
                                  xmpp_server_stanza_callback cb, void *data);

/**
 * Remove a callback from the list of routes for stanza delivery.
 *
 * @param server An XMPP server instance.
 * @param jid    The JID to receive stanzas for.  "*" strings serve as
 *               wildcards for any field.
 * @param cb     The callback to call when the server receives a stanza for
 *               this JID.
 * @param data   Arbitrary data to be passed to the callback function.
 */
void xmpp_server_del_stanza_route(struct xmpp_server *server,
                                  const struct jid *jid,
                                  xmpp_server_stanza_callback cb, void *data);

/**
 * Deliver a stanza to a registered callback function.
 *
 * @param stanza The XMPP stanza to route.
 * @return True if successfully handled, false if not.
 */
bool xmpp_server_route_stanza(const struct xmpp_server *server,
                              struct xmpp_stanza *stanza);

/**
 * Add a callback to handle IQ stanza to a particular namespace+tag name.
 *
 * Multiple callbacks can be registered matching the same namespace.  They will
 * be called in the order they are registered.
 *
 * @param server An XMPP server instance.
 * @param ns     The namespace + name string to receive IQs for.  This string
 *               will be copied, you own the original.
 * @param cb     The callback to call when the server receives a iq stanza for
 *               this namespace.
 * @param data   Arbitrary data to be passed to the callback function.
 */
void xmpp_server_add_iq_route(struct xmpp_server *server, const char *ns,
                              xmpp_server_stanza_callback cb, void *data);

/**
 * Deregister a callback for IQ stanzas.
 *
 * @param server An XMPP server instance.
 * @param ns     The namespace + name to deregister callbacks for.
 */
void xmpp_server_del_iq_route(struct xmpp_server *server, const char *ns,
                              xmpp_server_stanza_callback cb, void *data);

/**
 * Deliver an IQ stanza to a registered callback function.
 *
 * @param ns     The namespace + name string to deliver the stanza to.
 * @param stanza The stanza to deliver.
 * @return True if successfully handled, false if not.
 */
bool xmpp_route_iq(const struct xmpp_server *server,
                   struct xmpp_stanza *stanza);
