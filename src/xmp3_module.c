/**
 * xmp3 - XMPP Proxy
 * Copyright (c) 2012 Drexel University
 *
 * @file xmp3_module.h
 * Defines structures and functions for XMP3 extension modules.
 */

#include <uthash.h>

#include <tj_solibrary.h>

#include "log.h"
#include "utils.h"
#include "xmp3_module.h"

static const char *SYMBOL_NAME = "XMP3_MODULE";

struct module {
    char *name;
    struct xmp3_module *functions;
    void *data;
    bool started;

    UT_hash_handle hh;
};

struct xmp3_modules {
    struct module *map;
    tj_solibrary *solibrary;
};

struct xmp3_modules* xmp3_modules_new() {
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

bool xmp3_modules_load(struct xmp3_modules *modules, const char *path,
                       const char *name) {
    tj_solibrary_entry *entry = NULL;
    struct module *module = NULL;

    entry = tj_solibrary_load(modules->solibrary, path);
    if (entry == NULL) {
        log_err("Could not load module '%s'", path);
        goto error;
    }

    module = calloc(1, sizeof(*module));
    check_mem(module);

    module->started = false;
    STRDUP_CHECK(module->name, name);
    module->functions = tj_solibrary_entry_getSymbol(entry, SYMBOL_NAME);
    if (module->functions == NULL) {
        log_err("No symbol '%s' defined in '%s'", SYMBOL_NAME, path);
        goto error;
    }

    module->data = module->functions->mod_new();
    if (module->data == NULL) {
        log_err("Failed to instantiate module '%s'", path);
        goto error;
    }

    HASH_ADD_KEYPTR(hh, modules->map, module->name, strlen(module->name),
                    module);
    return true;

error:
    if (module != NULL) {
        if (module->name != NULL) {
            free(module->name);
        }
        if (module->data != NULL) {
            module->functions->mod_del(module->data);
        }
    };
    return false;
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
            // Don't want to stop stopping modules just because one failed.
            rv = false;
        }
        module->started = false;
    }
    return rv;
}
