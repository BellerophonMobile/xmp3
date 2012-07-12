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

#include <stdarg.h>

#include <utstring.h>

#include <tj_error.h>
#include <tj_buffer.h>

//----------------------------------------------------------------------
//----------------------------------------------------------------------
#define NUM_TJ_ERROR_CODES 11
static const char *k_tj_error_code_label[] =
  {
    "OK",
    "FAILURE",
    "NO MEMORY",
    "API MISUSE",
    "MISSING RESOURCE",
    "SERVICE ERROR",
    "MISSING SERVICE",
    "PARSING ERROR",
    "SOCKET ERROR",
    "DATABASE ERROR",
    "THREAD ERROR"
  };

//----------------------------------------------------------------------
//----------------------------------------------------------------------
struct tj_error {
  UT_string *m_msg;

  tj_error_code m_majorCode;
};


//--------------------------------------------------------------
/*
 * This guy is for the case when there isn't even enough memory
 * available to create a tj_error reporting the problem.
 *
 * The commented approach with the static UT_string is feasible, but
 * more susceptible to breakages than the current m_msg = 0 setup,
 * with no real benefit.
 */

// static const UT_string k_noMemoryUTStringHack =
//   {
//     .d = "No memory for error.",
//     .n = 21,
//     .i = 21,
//   };

static const char k_noMemoryMessage[] = "No memory for error.";
static const tj_error k_noMemoryHack =
  {
    //    .m_msg = (UT_string *) &k_noMemoryUTStringHack,
    .m_msg = 0,
  };

static const tj_error *TJ_ERROR_NO_MEMORY_OBJ = &k_noMemoryHack;


//----------------------------------------------------------------------
//----------------------------------------------------------------------
tj_error *
tj_error_create(tj_error_code code, char *fmt, ...)
{
  va_list ap;

  tj_error *x;
  if ((x = (tj_error *) malloc(sizeof(tj_error))) == 0) {
    return (tj_error *) TJ_ERROR_NO_MEMORY_OBJ;
  }

  x->m_majorCode =
    (code >= 0 && code <= NUM_TJ_ERROR_CODES) ? code : TJ_ERROR_FAILURE;

  utstring_new(x->m_msg);
  utstring_printf(x->m_msg, "[%s]: ", k_tj_error_code_label[x->m_majorCode]);
  va_start(ap,fmt);
  utstring_printf_va(x->m_msg,fmt,ap);
  va_end(ap);

  return x;

  // end tj_error_create
}

void
tj_error_finalize(tj_error *x)
{
  if (x == TJ_ERROR_NO_MEMORY_OBJ)
    return;

  if (x->m_msg != 0)
    utstring_free(x->m_msg);
  // end tj_error_finalize
}


//----------------------------------------------------------------------
//----------------------------------------------------------------------
void
tj_error_appendMessage(tj_error *x, char *fmt, ...)
{
  va_list ap;

  if (x->m_msg == 0) {
    TJ_ERROR("Attempted to append to tj_error with no message memory.");
    return;
  }

  utstring_printf(x->m_msg, "\n[%s]: ", k_tj_error_code_label[x->m_majorCode]);
  va_start(ap,fmt);
  utstring_printf_va(x->m_msg,fmt,ap);
  va_end(ap);

  // end tj_error_appendMessage
}

//----------------------------------------------------------------------
const char *
tj_error_getMessage(tj_error *x)
{
  return (x->m_msg == 0) ? k_noMemoryMessage : utstring_body(x->m_msg);
  // end tj_error_getMessage
}
