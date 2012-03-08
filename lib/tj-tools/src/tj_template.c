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

#include <stdlib.h>
#include <string.h>

#include "tj_template.h"

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

#ifndef TJ_PAGE_SIZE
#define TJ_PAGE_SIZE 1024
#endif

//----------------------------------------------------------------------
//----------------------------------------------------------------------
typedef struct tj_template_variable tj_template_variable;
struct tj_template_variable {
  char *m_label;
  tj_buffer *m_substitution;
  tj_template_variable *m_next;
  char m_tracking;
  char m_recurse;
};

struct tj_template_variables {
  tj_template_variable *m_variables;
};

//----------------------------------
tj_template_variable *
tj_template_variable_create(const char *label);
void
tj_template_variable_finalize(tj_template_variable *x);

tj_template_variable *
tj_template_variables_find(tj_template_variables *vars, const char *label);

//----------------------------------------------------------------------
//----------------------------------------------------------------------
tj_template_variables *
tj_template_variables_create(void)
{
  tj_template_variables *vars;
  if ((vars=malloc(sizeof(tj_template_variables))) == 0) {
    TJ_ERROR("No memory for tj_template_variables.");
    return 0;
  }
  vars->m_variables = 0;
  return vars;
  // end tj_template_variables
}

void
tj_template_variables_finalize(tj_template_variables *vars)
{
  tj_template_variable *var;
  while ((var = vars->m_variables) != 0) {
    vars->m_variables = var->m_next;
    tj_template_variable_finalize(var);
  }
  free(vars);
  // end tj_template_variables
}

//----------------------------------------------
tj_template_variable *
tj_template_variable_create(const char *label)
{
  tj_template_variable *v;
  if ((v = malloc(sizeof(tj_template_variable))) == 0) {
    TJ_ERROR("No memory for tj_template_variable.");
    return 0;
  }

  if ((v->m_substitution = tj_buffer_create(0)) == 0) {
    TJ_ERROR("No memory for tj_buffer.");
    free(v);
    return 0;
  }

  if ((v->m_label = strdup(label)) == 0) {
    TJ_ERROR("No memory for label.");
    free(v->m_substitution);
    free(v);
    return 0;
  }

  v->m_recurse = 0;
  v->m_next = 0;

  return v;
  // end tj_template_variable_create
}

void
tj_template_variable_finalize(tj_template_variable *x)
{
  tj_buffer_finalize(x->m_substitution);
  free(x->m_label);
  free(x);
  // end tj_template_variable_finalize
}

tj_template_variable *
tj_template_variables_find(tj_template_variables *vars, const char *label)
{
  tj_template_variable *v = vars->m_variables;
  while (v != 0 && strcmp(v->m_label, label)) {
    v = v->m_next;
  }
  return v;
  // end tj_template_variables_find
}

void
tj_template_variables_setRecurse(tj_template_variables *vars,
                                 const char *label, char recurse)
{
  tj_template_variable *v = tj_template_variables_find(vars, label);
  if (v != 0)
    v->m_recurse = recurse;
  // end tj_template_variables_setRecurse
}

//----------------------------------------------
int
tj_template_variables_setFromString(tj_template_variables *vars,
                                    const char *label,
                                    const char *substitution)
{
  tj_template_variable *v = tj_template_variables_find(vars, label);

  if (v == 0) {
    if ((v = tj_template_variable_create(label)) == 0) {
      return 0;
    }
    v->m_next = vars->m_variables;
    vars->m_variables = v;
    // end v==0
  } else
    tj_buffer_reset(v->m_substitution);

  if (!tj_buffer_append(v->m_substitution,
                        (tj_buffer_byte *) substitution,
                        strlen(substitution))) {
    TJ_ERROR("Could not append string to template variable.");
    return 0;
  }

  return 1;
  // end tj_template_variables_setFromString
}

int
tj_template_variables_setFromFileStream(tj_template_variables *vars,
                                        const char *label, FILE *substitution)
{
  tj_template_variable *v = tj_template_variables_find(vars, label);

  if (v == 0) {
    if ((v = tj_template_variable_create(label)) == 0) {
      return 0;
    }
    v->m_next = vars->m_variables;
    vars->m_variables = v;
    // end v==0
  }
  else
    tj_buffer_reset(v->m_substitution);

  if (!tj_buffer_appendFileStream(v->m_substitution, substitution)) {
    TJ_ERROR("Could not append file stream to template variable.");
    return 0;
  }

  return 1;
  // end tj_template_variables_setFromFileStream
}

int
tj_template_variables_setFromFile(tj_template_variables *vars,
                                  const char *label, const char *filename)
{
  FILE *fp;
  if ((fp = fopen(filename, "rb")) == 0) {
    TJ_ERROR("Could not open file %s for read.", filename);
  }

  if (!tj_template_variables_setFromFileStream(vars, label, fp)) {
    fclose(fp);
    TJ_ERROR("Could not append file to template variable.");
    return 0;
  }

  fclose(fp);

  return 1;
  // end tj_template_variables_setFromFile
}

//--------------------------------------------------------------
typedef enum {
  SCAN,
  MARK,
  TRACK
} tmpl_scan_mode;

int
tj_template_variables_apply(tj_template_variables *variables,
                            tj_buffer *dest,
                            tj_buffer *src)
{
  int tmplIndex = 0;
  int start = 0, end = 0;
  tmpl_scan_mode mode = SCAN;
  int varScanLen;

  tj_buffer_byte *template = tj_buffer_getBytes(src);
  tj_template_variable *v;

  while (tmplIndex < tj_buffer_getUsed(src)) {
    if (template[tmplIndex] == '$') {
      if (mode == SCAN) {
	mode = MARK;
      } else if (mode == MARK) {
        if (!tj_buffer_append(dest, &template[start], tmplIndex-start)) {
          TJ_ERROR("Could not append text before mark.");
          goto error;
        }
	mode = SCAN;
	start = tmplIndex+1;
	// end mode == MARK
      }
    } else {
      if (mode == MARK) {
        for (v = variables->m_variables; v != 0; v = v->m_next) {
          v->m_tracking = 1;
        }
	varScanLen = 0;
	end = tmplIndex-1; // Cannot happen before tmplIndex==1, so safe
	mode = TRACK;
      }

      if (mode == TRACK) {
	char alive = 0;

        for (v = variables->m_variables; v != 0; v = v->m_next) {
	  if (v->m_tracking) {

	    if (v->m_label[varScanLen] == 0) {
	      alive = 0;

              if (!tj_buffer_append(dest, &template[start], end-start)) {
                TJ_ERROR("Could not append pre-substitution text.");
                goto error;
              }

              if (v->m_recurse) {
                if (!tj_template_variables_apply(variables,
                                                 dest,
                                                 v->m_substitution)) {
                  TJ_ERROR("Could not recurse substitution.");
                  goto error;
                }
              } else {
                if (!tj_buffer_appendBuffer(dest, v->m_substitution)) {
                  TJ_ERROR("Could not append substitution.");
                  goto error;
                }
              }
	      start = tmplIndex;

	      break;
	    } else if (v->m_label[varScanLen] != template[tmplIndex]) {
	      v->m_tracking = 0;
	    } else {
              alive = 1;
            }
	  }

	  // end looping vars
	}

	if (alive) {
	  varScanLen++;
	} else {
	  mode = SCAN;
	}
	// end track
      }
    }

    tmplIndex++;
    // end looping over template
  }

  if (!tj_buffer_append(dest, &template[start], tmplIndex-start)) {
    TJ_ERROR("Could not append final chunk.");
    goto error;
  }

  return 1;

 error:
  return 0;
  // end tj_template_expand
}
