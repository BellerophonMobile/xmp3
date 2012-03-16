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

#ifndef __tj_solibrary_h__
#define __tj_solibrary_h__

//--------------------------------------------------------------
typedef struct tj_solibrary tj_solibrary;
typedef struct tj_solibrary_entry tj_solibrary_entry;

tj_solibrary *
tj_solibrary_create(void);
void
tj_solibrary_finalize(tj_solibrary *x);

tj_solibrary_entry *
tj_solibrary_getNext(tj_solibrary *x, tj_solibrary_entry *e);


tj_solibrary_entry *
tj_solibrary_load(tj_solibrary *x, const char *fn);

void *
tj_solibrary_entry_getSymbol(tj_solibrary_entry *x, const char *func);

#endif // __tj_solibrary_h__
