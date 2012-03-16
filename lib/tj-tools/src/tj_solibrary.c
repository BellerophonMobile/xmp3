/*
 * Copyright (c) 2012 Joe Kopena <tjkopena@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#include <utlist.h>

#include "tj_solibrary.h"

//----------------------------------------------------------------------
//----------------------------------------------------------------------
#ifndef TJ_LOG_STREAM
#define TJ_LOG_STREAM stdout
#endif

#ifndef TJ_ERROR_STREAM
#define TJ_ERROR_STREAM stderr
#endif

#ifndef TJ_LOG
#ifdef NDEBUG
#define TJ_LOG(M, ...)
#else
#define TJ_LOG(M, ...) fprintf(TJ_LOG_STREAM, "%s: " M "\n", __FUNCTION__, ##__VA_ARGS__)
#endif // ifndef NDEBUG else
#endif // ifndef TJ_LOG

#ifndef TJ_ERRROR
#define TJ_ERROR(M, ...) fprintf(TJ_ERROR_STREAM, "[ERROR] %s:%s:%d: " M "\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)
#endif


//----------------------------------------------
struct tj_solibrary_entry {
  char *m_fn;
  void *m_handle;
  tj_solibrary_entry *next;
};

struct tj_solibrary {
  tj_solibrary_entry *m_list;
};


//----------------------------------------------------------------------
//----------------------------------------------------------------------
tj_solibrary *
tj_solibrary_create(void)
{
  TJ_LOG("Create.");
  tj_solibrary *x;
  if ((x = malloc(sizeof(tj_solibrary))) == 0) {
    TJ_ERROR("Could not allocate tj_solibrary.");
    return 0;
  }
  x->m_list = 0;
  return x;
  // end tj_solibrary_create
}

void
tj_solibrary_finalize(tj_solibrary *x)
{
  TJ_LOG("Finalize.");

  tj_solibrary_entry *e;
  while ((e = x->m_list) != 0) {
    x->m_list = x->m_list->next;
    TJ_LOG("  %s", e->m_fn);
    free(e->m_fn);
    dlclose(e->m_handle);
    free(e);
  }

  free(x);
  // end tj_solibrary_finalize
}

tj_solibrary_entry *
tj_solibrary_getNext(tj_solibrary *x, tj_solibrary_entry *e)
{
  if (e == 0)
    return x->m_list;

  return e->next;
  // end tj_solibrary_getNext
}

tj_solibrary_entry *
tj_solibrary_load(tj_solibrary *x, const char *fn)
{
  char *error;

  tj_solibrary_entry *e;
  if ((e = malloc(sizeof(tj_solibrary_entry))) == 0) {
    TJ_ERROR("Could not allocate tj_solibrary_entry.");
    return 0;
  }

  if ((e->m_fn = strdup(fn)) == 0) {
    TJ_ERROR("Could not allocate tj_solibrary_entry fn.");
    free(e);
    return 0;
  }

  dlerror();
  e->m_handle = dlopen(fn, RTLD_LAZY);
  error = dlerror();
  if (e->m_handle == 0 || error != 0) {
    TJ_ERROR("Could not open lib file %s:\n%s", fn, error);
    free(e->m_fn);
    free(e);
    return 0;
  }

  LL_PREPEND(x->m_list, e);

  return e;
    
  // end tj_solibrary_load
}

void *
tj_solibrary_entry_getSymbol(tj_solibrary_entry *x, const char *func)
{
  char *error;
  void *handle;

  dlerror();
  handle = dlsym(x->m_handle, func);
  error = dlerror();
  if (handle == 0 || error != 0) {
    TJ_ERROR("Could not find symbol %s in library %s:\n%s",
	     func, x->m_fn, error);
    return 0;
  }

  return handle;

  // end tj_solibrary_entry_getSymbol
}
