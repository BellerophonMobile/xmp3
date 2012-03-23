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
 * @file event.h
 * Simple event loop
 */

#pragma once

/** Opaque type representing an event loop. */
struct event_loop;

/**
 * Event callback function definition.
 *
 * @param loop Pointer to the event loop object
 * @param fd   The file descriptor that is ready to read
 * @param data User-defined data
 */
typedef void (*event_callback)(struct event_loop *loop, int fd, void *data);

/** Allocate a new event loop.
 *
 * @return A new event loop ready to use.
 */
struct event_loop* event_loop_new(void);

/** Free an existing event loop.
 *
 * @param loop An event loop.
 */
void event_loop_del(struct event_loop *loop);

/**
 * Register a callback function for read events on a file-descriptor.
 *
 * @param loop Existing event loop.
 * @param fd   File descriptor to watch for new data on.
 * @param func Function called when data is ready to read.
 * @param data Arbitrary data passed to the callback function.
 */
void event_register_callback(struct event_loop *loop, int fd,
                             event_callback func, void *data);

/**
 * Deregisters a callback for a particular file descriptor.
 *
 * @param loop Existing event loop.
 * @param fd   File descriptor to cancel notifications for.
 */
void event_deregister_callback(struct event_loop *loop, int fd);

/**
 * Start the event loop.
 *
 * This function will not return unless cancelled from a callback.
 *
 * @param loop Existing event loop to start.
 */
void event_loop_start(struct event_loop *loop);

/**
 * Stop the event loop.
 *
 * This function can be called from a callback or a signal handler.
 *
 * @param loop Existing event loop to start.
 */
void event_loop_stop(struct event_loop *loop);
