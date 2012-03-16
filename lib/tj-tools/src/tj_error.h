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

#ifndef __tj_error_h__
#define __tj_error_h__

#include <stdio.h>

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

#define TJ_ERROR_OK (0)

typedef enum tj_error_code tj_error_code;
enum tj_error_code {
  TJ_ERROR_NO_ERROR,
  TJ_ERROR_FAILURE,
  TJ_ERROR_NO_MEMORY,
  TJ_ERROR_API_MISUSE,
  TJ_ERROR_MISSING_RESOURCE,
  TJ_ERROR_SERVICE,
  TJ_ERROR_MISSING_SERVICE,
  TJ_ERROR_PARSING,
  TJ_ERROR_SOCKET,
  TJ_ERROR_DATABASE,
  TJ_ERROR_THREAD
};


//----------------------------------------------------------------------
//----------------------------------------------------------------------
typedef struct tj_error tj_error;

tj_error *
tj_error_create(tj_error_code code, char *fmt, ...);

void
tj_error_finalize(tj_error *x);

void
tj_error_appendMessage(tj_error *x, char *fmt, ...);

const char *
tj_error_getMessage(tj_error *x);


//----------------------------------------------------------------------

#endif // __tj_error_h__
