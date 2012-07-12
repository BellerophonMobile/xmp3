/*
 * Copyright (c) 2011 Joe Kopena <tjkopena@gmail.com>
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

#ifndef __tj_heap_h__
#define __tj_heap_h__

#include <stddef.h>

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


//----------------------------------------------------------------------
//----------------------------------------------------------------------

/**
 * See documentation in the tj-tools wiki on how to use a tj_heap.
 *           http://code.google.com/p/tj-tools/wiki/tj_heap
 */

#define TJ_HEAP_DECL(type, keytype, valuetype, comparator)              \
  typedef struct { keytype m_key;                                       \
                   valuetype m_value;                                   \
                 } type##_element;                                      \
  typedef struct { type##_element *m_array;                             \
                   size_t m_n; size_t m_used;                           \
                 } type;                                                \
  int (*type##_cmp)(keytype a, keytype b) = &comparator;                \
  type *                                                                \
  type##_create(int initial)                                            \
  {                                                                     \
    type *t;                                                            \
    if ((t = malloc(sizeof(type))) == 0) {                              \
      TJ_ERROR("Could not allocate " #type ".");                        \
      return 0;                                                         \
    }                                                                   \
    if ((t->m_array = malloc(sizeof(type##_element) * initial)) == 0) { \
      TJ_ERROR("Could not allocate " #type "_element[%d].", initial);   \
      free(t);                                                          \
      return 0;                                                         \
    }                                                                   \
    t->m_n = initial;                                                   \
    t->m_used = 0;                                                      \
    return t;                                                           \
  }                                                                     \
  void                                                                  \
  type##_finalize(type *x)                                              \
  {                                                                     \
    free(x->m_array);                                                   \
    free(x);                                                            \
  }                                                                     \
  int                                                                   \
  type##_add(type *h, keytype k, valuetype v)                           \
  {                                                                     \
    if (h->m_used == h->m_n) {                                          \
      type##_element *oa;                                               \
      if ((h->m_array = realloc(oa=h->m_array,                          \
                                sizeof(type##_element) *                \
                                (h->m_n*2))) == 0) {                    \
        TJ_ERROR("Could not reallocate " #type "_element[%zu].",        \
                 h->m_n*2);                                             \
        h->m_array = oa;                                                \
        return 0;                                                       \
      }                                                                 \
      h->m_n *= 2;                                                      \
    }                                                                   \
    size_t newindex = h->m_used, parentindex = h->m_used/2;             \
    while (newindex>0 && type##_cmp(k,h->m_array[parentindex].m_key)) { \
      h->m_array[newindex].m_key = h->m_array[parentindex].m_key;       \
      h->m_array[newindex].m_value = h->m_array[parentindex].m_value;   \
      newindex = parentindex;                                           \
      parentindex = newindex/2;                                         \
    }                                                                   \
    h->m_array[newindex].m_key = k;                                     \
    h->m_array[newindex].m_value = v;                                   \
    h->m_used++;                                                        \
    return 1;                                                           \
  }                                                                     \
  int                                                                   \
  type##_peek(type *h, keytype *k, valuetype *v)                        \
  {                                                                     \
    if (h->m_used == 0) return 0;                                       \
    (*k)=h->m_array[0].m_key;                                           \
    (*v)=h->m_array[0].m_value;                                         \
    return 1;                                                           \
  }                                                                     \
  int                                                                   \
  type##_remove(type *h, int index, keytype *k, valuetype *v)		\
  {                                                                     \
    if (index >= h->m_used || index < 0) return 0;                      \
    (*k)=h->m_array[index].m_key;                                       \
    (*v)=h->m_array[index].m_value;                                     \
    h->m_used--;                                                        \
    h->m_array[index].m_key = h->m_array[h->m_used].m_key;              \
    h->m_array[index].m_value = h->m_array[h->m_used].m_value;          \
    if ((index*2)+1 < h->m_used) {					\
      int root = index, child = (root*2)+1;				\
      keytype kt;                                                       \
      valuetype vt;                                                     \
      do {                                                              \
        if (child < h->m_used - 1 &&                                    \
            type##_cmp(h->m_array[child+1].m_key,                       \
                       h->m_array[child].m_key))                        \
          child++;                                                      \
        if (type##_cmp(h->m_array[root].m_key,                          \
                       h->m_array[child].m_key)) break;                 \
        kt = h->m_array[root].m_key;                                    \
        h->m_array[root].m_key = h->m_array[child].m_key;               \
        h->m_array[child].m_key = kt;                                   \
        vt = h->m_array[root].m_value;                                  \
        h->m_array[root].m_value = h->m_array[child].m_value;           \
        h->m_array[child].m_value = vt;                                 \
        root = child;                                                   \
        child = (root*2) + 1;                                           \
      } while (child < h->m_used);                                      \
    }                                                                   \
    return 1;                                                           \
  }                                                                     \
  int                                                                   \
  type##_pop(type *h, keytype *k, valuetype *v)                         \
  {                                                                     \
    return type##_remove(h, 0, k, v);                                   \
  }                                                                     \
  int                                                                   \
  type##_find(type *h, int (*test)(void *h, keytype k, valuetype v),    \
             void *data)                                                \
  {                                                                     \
    int i = 0;                                                          \
    while (i < h->m_used &&                                             \
           !test(data, h->m_array[i].m_key, h->m_array[i].m_value))     \
      i++;                                                              \
    if (i == h->m_used)                                                 \
      return -1;                                                        \
    return i;                                                           \
  }

//----------------------------------------------
#define TJ_HEAP_IMPL(type)

#endif // __tj_heap_h__
