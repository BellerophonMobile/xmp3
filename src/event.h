/**
 * xmp3 - XMPP Proxy
 * event.{c,h} - Simple event loop
 * Copyright (c) 2011 Drexel University
 */

#pragma once

/** Opaque type representing an event loop */
struct event_loop;

/**
 * Callback function definition
 *
 * struct event_loop *loop - Pointer to the event loop object
 * int fd                  - The file descriptor that is ready to read
 * void *data              - User-defined data
 */
typedef void (*event_callback)(struct event_loop *loop, int fd, void *data);

/** Allocate a new event loop */
struct event_loop* event_new_loop(void);

/** Free an existing event loop */
void event_del_loop(struct event_loop *loop);

/**
 * Register a callback function for read events on a file-descriptor
 *
 * struct event_loop *loop - Existing event loop
 * int fd                  - File descriptor to watch for new data on
 * event_callback func     - Function called when data is ready to read
 * void *data              - Arbitrary data passed to the callback function
 */
void event_register_callback(struct event_loop *loop, int fd,
                             event_callback func, void *data);

/**
 * Deregisters a callback for a particular file descriptor
 *
 * struct event_loop *loop - Existing event loop
 * int fd                  - File descriptor to cancel notifications for
 */
void event_deregister_callback(struct event_loop *loop, int fd);

/**
 * Start the event loop
 *
 * This function will not return unless cancelled from a callback
 *
 * struct event_loop *loop - Existing event loop to start
 */
void event_loop_start(struct event_loop *loop);

/**
 * Stop the event loop.
 *
 * This function can be called from a callback or a signal handler.
 *
 * struct event_loop *loop - Existing event loop to start.
 */
void event_loop_stop(struct event_loop *loop);
