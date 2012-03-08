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

#ifndef __tj_buffer_h__
#define __tj_buffer_h__

#include <stdio.h>

//----------------------------------------------------------------------
//----------------------------------------------------------------------
typedef unsigned char           tj_buffer_byte;
typedef struct tj_buffer        tj_buffer;

/**
 * Create a tj_buffer.  Data can be added to a tj_buffer and it will
 * grow, if possible, to accommodate.  The buffer can then be reset
 * and the memory reused.  Note that none of the tj_buffer operations
 * check if the passed tj_buffer * is null.  Operation on buffers
 * created statically are also undefined.  All operations assume the
 * structure has been created using tj_buffer_create.
 *
 * \param n The initial buffer size; can be 0.  If you write directly
 * into the buffer rather than use tj_buffer_append, it must be the
 * total length.
 */
tj_buffer *
tj_buffer_create(size_t initial);

/**
 * Destroys a buffer and frees its memory.  Behavior of any future
 * calls on the buffer are undefined, but will probably segfault.
 *
 * \param x The buffer to deallocate.
 */
void
tj_buffer_finalize(tj_buffer *x);

/**
 * Set whether or not the buffer owns its data and should free it when
 * the tj_buffer is finalized.
 *
 * \param b The buffer to operate on.
 * \param own 0 for the buffer no longer owns and should not free its
 * data, 1 otherwise.
 */
void
tj_buffer_setOwnership(tj_buffer *b, char own);

/**
 * Reset the buffer but do not release the memory.  Future calls to
 * tj_buffer_append() overwrite previous contents but reuse the
 * current memory allocation.
 *
 * \param b The buffer to operate on.
 */
void
tj_buffer_reset(tj_buffer *b);

/**
 * Get the currently used extent of the buffer.
 *
 * \param b The buffer to operate on.
 * 
 * \return The number of bytes consumed by data since creation or the
 * last reset.
 */
size_t
tj_buffer_getUsed(tj_buffer *b);

/**
 * Get how much memory is currently allocated for the buffer.
 * 
 * \param b The buffer to operate on.
 *
 * \return The current total internal memory allocation for the
 * buffer.
 */
size_t
tj_buffer_getAllocated(tj_buffer *b);

/**
 * Get a pointer to the internal byte array.
 *
 * \param b The buffer to operate on.
 *
 * \return
 */
tj_buffer_byte *
tj_buffer_getBytes(tj_buffer *b);

/**
 * Get a pointer to the internal byte array as a string.  Same as
 * (char *) tj_buffer_getBytes(b).  This does not ensure a null
 * terminator is present.
 *
 * \param b The buffer to operate on.
 *
 * \return
 */
char *
tj_buffer_getAsString(tj_buffer *b);

/**
 * Get a pointer to the internal byte array from a given position.
 * This is no different from tj_buffer_getBytes(b)+i.
 *
 * \param b The buffer to operate on.
 *
 * \return
 */
tj_buffer_byte *
tj_buffer_getBytesAtIndex(tj_buffer *b, size_t i);

/**
 * Write data into a buffer, growing its memory allocation if
 * necessary.  The new data is pushed onto the end of the buffer.  If
 * the internal memory allocation cannot be grown to encompass all of
 * the data, none of it is written and the previous buffer contents
 * and size are maintained.
 *
 * \param b The buffer to operate on.
 * \param data A byte array of at least length n.
 * \param n The number of bytes from data to add to b.
 *
 * \return 0 on failure, 1 otherwise.
 */
int
tj_buffer_append(tj_buffer *b, tj_buffer_byte *data, size_t n);

/**
 * Appends the used extent of s into b.  Follows the same memory rules
 * as tj_buffer_append().
 *
 * \param b The buffer to operate on.
 * \param s The buffer providing data to append.
 *
 * \return 0 on failure, 1 otherwise.
 */
int
tj_buffer_appendBuffer(tj_buffer *b, tj_buffer *s);

/**
 * Add a string to the end of the buffer, including the null
 * terminator, growing the buffer allocation if necessary.  If the
 * internal memory allocation cannot be grown to encompass all of the
 * data, none of it is written and the previous buffer contents and
 * size are maintained.  Nothing is done to the existing contents of
 * the buffer.  In particular, this means that if the existing buffer
 * already has a null terminator, functions like strcmp() will only
 * see the portion up to that terminator.  tj_buffer_appendAsString is
 * intended to be used for iterated string construction.
 *
 * \param b The buffer to operate on.
 * \param str Null terminated string.
 *
 * \return 0 on failure, 1 otherwise.
 */
int
tj_buffer_appendString(tj_buffer *b, const char *str);

/**
 * Add a string to the end of the buffer, including the null
 * terminator, growing the buffer allocation if necessary.  If the
 * internal memory allocation cannot be grown to encompass all of the
 * data, none of it is written and the previous buffer contents and
 * size are maintained.  The last byte of the previous buffer is
 * assumed to be a null terminator and is overwritten.  This function
 * can therefore be called iteratively to construct a string.
 *
 * \param b The buffer to operate on.
 * \param str Null terminated string.
 *
 * \return 0 on failure, 1 otherwise.
 */
int
tj_buffer_appendAsString(tj_buffer *b, const char *str);


/**
 * Read a file or file stream into the buffer.  The given file handle
 * can be a stream such as stdin, but the entire contents will be read
 * before the function returns.  Internally, the function reads bytes
 * in TJ_PAGE_SIZE sized chunks, which may be redefined at compile
 * time.  Note that no null terminator is included.  I.e., to read a
 * text file from f and interpret as a string, read the file using
 * tj_buffer_appendFileStream(b, f), and then call
 * tj_buffer_appendString(b, "") to append a null terminator.
 *
 * \param b The buffer to operate on.
 * \param fh An open file descriptor to read from.
 *
 * \return 0 on failure, 1 otherwise.
 */
int
tj_buffer_appendFileStream(tj_buffer *b, FILE *fh);

/**
 * Read a file into the buffer.  The given filename is opened in
 * binary mode and read in using tj_buffer_appendFileStream().  The
 * same notes about null terminators apply.
 *
 * \param b The buffer to operate on.
 * \param fh Filename to open.
 *
 * \return 0 on failure, 1 otherwise.
 */
int
tj_buffer_appendFile(tj_buffer *b, const char *filename);

#endif // __tj_buffer_h__
