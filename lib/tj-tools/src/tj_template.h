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

#ifndef __tj_template_h__
#define __tj_template_h__

#include "tj_buffer.h"

//----------------------------------------------------------------------
typedef struct tj_template_variables tj_template_variables;

/**
 * Create a tj_template_variables object.  It will initially contain
 * no substitutions.
 */
tj_template_variables *
tj_template_variables_create(void);

/**
 * Destroy a tj_template_variables object, deallocating it and any
 * substitutions that have been added to it.
 */
void
tj_template_variables_finalize(tj_template_variables *vars);

/**
 * Mark whether or not a variable should be applied recursively.
 * Recursive substitutions are scanned for variables which are again
 * expanded when the variable is expanded.  There is no infinite loop
 * detection!  The default is non-recursive.
 *
 * \param vars The substitution container.
 * \param label The variable in question.
 * \param recurse Whether (1) or not (0) the variable should recurse.
 */
void
tj_template_variables_setRecurse(tj_template_variables *vars,
                                 const char *label, char recurse);

/**
 * Define a substitution as a string.  The caller maintains ownership
 * of the variable and substitution memory.  If a value was already
 * set for the variable, it is replaced.
 *
 * \param vars The substitution container.
 * \param label The variable to define.
 * \param substitution The string value which will replace the variable.
 * \return 0 on failure, 1 otherwise.
 */
int
tj_template_variables_setFromString(tj_template_variables *vars,
                                    const char *label,
                                    const char *substitution);

/**
 * Define a substitution as the contents of a file.  The caller
 * maintains ownership of the variable and FILE *.  If a value was
 * already set for the variable, it is replaced.  This function is
 * safe to use on stdin and other streams.
 *
 * \param vars The substitution container.
 * \param label The variable to define.
 * \param substitution The stream whose contents define the variable.
 * \return 0 on failure, 1 otherwise.
 */
int
tj_template_variables_setFromFileStream(tj_template_variables *vars,
                                        const char *label, FILE *substitution);

/**
 * Define a substitution as the contents of a file.  The caller
 * maintains ownership of the variable and filename.  If a value was
 * already set for the variable, it is replaced.  Internally this
 * function loads the given file (as binary), and calls
 * tj_template_variables_setFromFileStream.
 *
 * \param vars The substitution container.
 * \param label The variable to define.
 * \param filename The file whose contents define the variable.
 * \return 0 on failure, 1 otherwise.
 */
int
tj_template_variables_setFromFile(tj_template_variables *vars,
                                  const char *label, const char *filename);

/**
 * Expand a set of substitutions into a buffer, using another buffer
 * as the template guiding substitution.  Caller maintains ownership
 * of all objects.
 *
 * \param vars The substitutions to apply.
 * \param dest The buffer into which the expansion is conducted.
 * \param src Buffer containing the template to expand.
 * \return 0 on failure, 1 otherwise.
 */
int
tj_template_variables_apply(tj_template_variables *variables,
                            tj_buffer *dest,
                            tj_buffer *src);

#endif // __tj_template_h__
