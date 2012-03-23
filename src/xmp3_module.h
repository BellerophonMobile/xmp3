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

/** Called to instantiate a new intance of the module. */
typedef void* (*xmp3_module_new)(void);

/** Called when the module should be deleted. */
typedef void (*xmp3_module_del)(void *module);

/** Called for each key=value pair in the config file for this module. */
typedef bool (*xmp3_module_conf)(void *module, const char *key,
                                 const char *value);

/** Called when the server is starting. */
typedef bool (*xmp3_module_start)(void *module, struct xmpp_server *server);

/** Called when the server is stopping. */
typedef bool (*xmp3_module_stop)(void *module);

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
    xmp3_module_new mod_new;
    xmp3_module_del mod_del;
    xmp3_module_conf mod_conf;
    xmp3_module_start mod_start;
    xmp3_module_stop mod_stop;
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
