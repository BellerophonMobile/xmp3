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
#include <unistd.h>

#include <utlist.h>

#include "tj_searchpathlist.h"

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
typedef struct tj_searchpathlist_entry tj_searchpathlist_entry;
struct tj_searchpathlist_entry {
  char *m_path;
  tj_searchpathlist_entry *next;
};

struct tj_searchpathlist {
  tj_searchpathlist_entry *m_list;
};


//----------------------------------------------------------------------
//----------------------------------------------------------------------
tj_searchpathlist *
tj_searchpathlist_create(void)
{
  TJ_LOG("Create.");
  tj_searchpathlist *x;
  if ((x = malloc(sizeof(tj_searchpathlist))) == 0) {
    TJ_ERROR("Could not malloc tj_searchpathlist.");
    return 0;
  }

  x->m_list = 0;

  return x;
  // end tj_searchpathlist_create
}

void
tj_searchpathlist_finalize(tj_searchpathlist *x)
{
  TJ_LOG("Finalize.");
  tj_searchpathlist_entry *e;
  while ((e = x->m_list) != 0) {
    x->m_list = x->m_list->next;
    TJ_LOG("  %s", e->m_path);
    free(e->m_path);
    free(e);
  }

  free(x);
  // end tj_searchpathlist_finalize
}

int
tj_searchpathlist_add(tj_searchpathlist *x, const char *path)
{
  TJ_LOG("Add %s", path);

  tj_searchpathlist_entry *e;

  if ((e = malloc(sizeof(tj_searchpathlist_entry))) == 0) {
    TJ_ERROR("Could not allocate tj_searchpathlist_entry.");
    return 0;
  }

  if ((e->m_path = strdup(path)) == 0) {
    TJ_ERROR("Could not strdup tj_searchpathlist_entry path.");
    free(e);
    return 0;
  }

  LL_APPEND(x->m_list, e);

  return 1;

  // end tj_searchpathlist_add
}

int
tj_searchpathlist_locate(tj_searchpathlist *x, const char *fn, char *result,
                         int n)
{
  tj_searchpathlist_entry *e = x->m_list;
  while (e != 0) {
    snprintf(result, n, "%s/%s", e->m_path, fn);
    TJ_LOG("Found %s at %s.", fn, result);
    if (access(result, R_OK) == 0)
      return 1;
    e = e->next;
  }

  return 0;
  // end tj_searchpathlist_locate
}
