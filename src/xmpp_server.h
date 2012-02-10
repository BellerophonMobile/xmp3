/**
 * xmp3 - XMPP Proxy
 * xmpp_server.{c,h} - Main XMPP server data/functions
 * Copyright (c) 2011 Drexel University
 * @file
 */

#pragma once

#include <stdbool.h>

#include <openssl/ssl.h>

// Forward declarations
struct event_loop;
struct xmp3_options;
struct xmpp_client;
struct xmpp_server;
struct xmpp_stanza;
struct xmpp_client_iterator;

/**
 * Callback to deliver an XMPP stanza.
 *
 * @param stanza The message stanza to be delivered.
 * @param data   Data from when this callback was registered.
 * @return True if message was handled, false if not.
 */
typedef bool (*xmpp_server_stanza_callback)(struct xmpp_stanza *stanza,
                                            struct xmpp_server *server,
                                            void *data);

/**
 * Callback to notify components of a client disconnecting.
 *
 * @param client The client that is about to/has been disconnected.
 * @param data   Data from when this callback was registered.
 */
typedef void (*xmpp_server_client_callback)(struct xmpp_client *client,
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

/** Gets the event loop instance this server is using. */
struct event_loop* xmpp_server_loop(const struct xmpp_server *server);

/** Gets the JID of this server. */
const struct jid* xmpp_server_jid(const struct xmpp_server *server);

/** Gets the SSL context if there is one, else NULL. */
SSL_CTX* xmpp_server_ssl_context(const struct xmpp_server *server);

/**
 * Add a callback to be notified when a client disconnects.
 *
 * This should be used in components in order to clean up after a client.  You
 * can use this to attempt to send some sort of final message to a client,
 * but its socket is not guaranteed to still be connected.
 *
 * @param server An XMPP server instance.
 * @param client The client to register notifications for.
 * @param cb     The callback function to call.
 * @param data   Arbitrary data to be passed to the callback function.
 */
void xmpp_server_add_client_listener(struct xmpp_client *client,
                                     xmpp_server_client_callback cb,
                                     void *data);

/** Remove a callback from the list of disconnect notifications. */
void xmpp_server_del_client_listener(struct xmpp_client *client,
                                     xmpp_server_client_callback cb,
                                     void *data);

/**
 * Attempt to cleanly disconnect a client, and clean up its resources.
 *
 * This can be called by any callbacks in order to disconnect a client if some
 * sort of error occurs.
 */
void xmpp_server_disconnect_client(struct xmpp_client *client);

/**
 * Find a locally connected client by JID.
 *
 * @returns The connected client instance if found, NULL if not.
 */
struct xmpp_client* xmpp_server_find_client(const struct xmpp_server *server,
                                            const struct jid *jid);

struct xmpp_client_iterator* xmpp_client_iterator_new(
        const struct xmpp_server *server);

struct xmpp_client* xmpp_client_iterator_next(
        struct xmpp_client_iterator *iter);

void xmpp_client_iterator_del(struct xmpp_client_iterator *iter);

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
 * @param server The server to process the stanza.
 * @param stanza The XMPP stanza to route.
 * @return True if successfully handled, false if not.
 */
bool xmpp_server_route_stanza(struct xmpp_server *server,
                              struct xmpp_stanza *stanza);

/**
 * Add a callback to handle IQ stanza to a particular namespace+tag name.
 *
 * Multiple callbacks can be registered matching the same namespace.  They will
 * be called in the order they are registered.
 *
 * @param server An XMPP server instance.
 * @param ns     The namespace string to receive IQs for.  This string will be
 *               copied, you own the original.
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
 * @param stanza The stanza to deliver.
 * @return True if successfully handled, false if not.
 */
bool xmpp_server_route_iq(struct xmpp_server *server,
                          struct xmpp_stanza *stanza);
