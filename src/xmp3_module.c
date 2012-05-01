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
 * @file xmp3_module.c
 * Defines structures and functions for XMP3 extension modules.
 */

#include <uthash.h>

#include <tj_solibrary.h>

#include "log.h"
#include "utils.h"
#include "xmp3_module.h"

static const char *SYMBOL_NAME = "XMP3_MODULE";

/** Structure representing a loaded module. */
struct module {
    /** The name of this module instance. */
    char *name;

    /** The functions to interact with this module. */
    struct xmp3_module *functions;

    /** The module instance. */
    void *data;

    /** Whether this module has been started or not. */
    bool started;

    /** These are kept in a hash table. */
    UT_hash_handle hh;
};

struct xmp3_modules {
    struct module *map;
    tj_solibrary *solibrary;
};

struct xmp3_modules* xmp3_modules_new(void) {
    struct xmp3_modules *modules = calloc(1, sizeof(*modules));
    check_mem(modules);

    modules->solibrary = tj_solibrary_create();
    return modules;
}

void xmp3_modules_del(struct xmp3_modules *modules) {
    xmp3_modules_stop(modules);

    struct module *module, *tmp;
    HASH_ITER(hh, modules->map, module, tmp) {
        HASH_DEL(modules->map, module);
        module->functions->mod_del(module->data);
        free(module->name);
        free(module);
    }

    tj_solibrary_finalize(modules->solibrary);
    free(modules);
}

bool xmp3_modules_add(struct xmp3_modules *modules, const char *name,
                      struct xmp3_module *funcs) {
    struct module *module = calloc(1, sizeof(*module));
    check_mem(module);

    module->started = false;
    STRDUP_CHECK(module->name, name);
    module->functions = funcs;
    module->data = module->functions->mod_new();
    if (module->data == NULL) {
        log_err("Failed to instantiate module '%s'", name);
        goto error;
    }

    HASH_ADD_KEYPTR(hh, modules->map, module->name, strlen(module->name),
                    module);

    return true;

error:
    if (module->name != NULL) {
        free(module->name);
    }
    return false;
}

bool xmp3_modules_load(struct xmp3_modules *modules, const char *path,
                       const char *name) {
    tj_solibrary_entry *entry = tj_solibrary_load(modules->solibrary, path);
    if (entry == NULL) {
        log_err("Could not load module '%s'", path);
        return false;
    }

    struct xmp3_module *funcs = tj_solibrary_entry_getSymbol(
            entry, SYMBOL_NAME);
    if (funcs == NULL) {
        log_err("No symbol '%s' defined in '%s'", SYMBOL_NAME, path);
        return false;
    }

    return xmp3_modules_add(modules, name, funcs);
}

bool xmp3_modules_config(struct xmp3_modules *modules, const char *name,
                         const char *key, const char *value) {
    struct module *module = NULL;
    HASH_FIND_STR(modules->map, name, module);
    if (module == NULL) {
        log_err("Module '%s' not loaded", name);
        return false;
    }
    return module->functions->mod_conf(module->data, key, value);
}

bool xmp3_modules_start(struct xmp3_modules *modules,
                        struct xmpp_server *server) {
    struct module *module, *tmp;
    HASH_ITER(hh, modules->map, module, tmp) {
        if (!module->functions->mod_start(module->data, server)) {
            log_err("Error starting module '%s'", module->name);
            return false;
        }
        module->started = true;
    }
    return true;
}

bool xmp3_modules_stop(struct xmp3_modules *modules) {
    bool rv = true;
    struct module *module, *tmp;
    HASH_ITER(hh, modules->map, module, tmp) {
        if (!module->started) {
            continue;
        }
        if (!module->functions->mod_stop(module->data)) {
            log_err("Error stopping module '%s'", module->name);
            /* Don't want to stop stopping modules just because one failed. */
            rv = false;
        }
        module->started = false;
    }
    return rv;
}
