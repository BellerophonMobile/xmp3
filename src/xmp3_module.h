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
 * @file xmp3_module.h
 * Defines structures and functions for XMP3 extension modules.
 */

#pragma once

#include <stdbool.h>

/* Forward declarations. */
struct xmpp_server;
struct xmp3_modules;

/**
 * Structure representing an XMP3 extension module.
 *
 * Each module should define a global symbol named "XMP3_MODULE" containing
 * this structure, with the methods filled in.  XMP3 will call these functions
 * at the appropriate time.
 *
 * To load a module, define a section in a config file with the name of the
 * shared object containing the extension.  See the example config file.
 */
struct xmp3_module {
    /**
     * Function for creating and initializing a new instance of a module.
     *
     * This will be called once for each new instantiation of a module.  It
     * should allocate memory and initialize any default variables for your
     * module, and return a pointer to that memory.  The returned pointer will
     * be passed to the subsequent functions to manipulate.
     *
     * @returns A new instance of this module.
     */
    void* (*mod_new)(void);

    /**
     * Function for cleaning up and deallocating an instance of a module.
     *
     * Thsi will be called once for each instantiated module when the server
     * shuts down.  It should free any resources allocated during the operation
     * of the module.
     *
     * @param module A pointer to the module instance.
     */
    void (*mod_del)(void *module);

    /**
     * Function for setting module options.
     *
     * This will be called once for each (key = value) pair in the
     * configuration file for a module instance.
     *
     * @param module A pointer to the module instance.
     * @param key    The name of the configuration option to set.
     * @param value  The value of the configuration option to set.
     * @returns True if successful, False on error (invalid key, etc.).
     */
    bool (*mod_conf)(void *module, const char *key, const char *value);

    /**
     * Function to start an instantiated module.
     *
     * This will be called immediately after the server is started, after all
     * the configuration options have been parsed.  This is where a module can
     * register stanza routes, callback functions, or run its own event loop in
     * a thread if necessary.  If a module fails to start, the server will stop
     * and report the error.
     *
     * @param module A pointer to the module instance.
     * @param server A pointer to the running XMPP server instance.
     * @returns True if sucessful, False on error.
     */
    bool (*mod_start)(void *module, struct xmpp_server *server);

    /**
     * Function to stop an instantiated and running module.
     *
     * This will be called just after the server is stopped.  This should
     * remove any registered stanza routes and event callbacks, and stop any
     * threads that are running.  Essesntially, this module should stop
     * everything short of freeing memory.  On error, the server will report
     * the error, but continue in its normal shutdown sequence.  In the future,
     * the server may have the capability to start/stop modules at runtime.
     *
     * @param module A pointer to the module instance.
     * @returns True if sucessful, False on error.
     */
    bool (*mod_stop)(void *module);
};

struct xmp3_modules* xmp3_modules_new(void);

void xmp3_modules_del(struct xmp3_modules *modules);

bool xmp3_modules_load(struct xmp3_modules *modules, const char *path,
                       const char *name);

bool xmp3_modules_config(struct xmp3_modules *modules, const char *name,
                         const char *key, const char *value);

bool xmp3_modules_start(struct xmp3_modules *modules,
                        struct xmpp_server *server);

bool xmp3_modules_stop(struct xmp3_modules *modules);
