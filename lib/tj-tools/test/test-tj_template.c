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

#include <stdio.h>
#include <string.h>

#include "tj_buffer.h"
#include "tj_template.h"

int fail = 0;

#define FAIL(M, ...) {fail = 1; printf("FAIL: " M "\n", ##__VA_ARGS__);}

int
main(int argc, char *argv[])
{

  tj_buffer *target = tj_buffer_create(0);
  tj_buffer *src = tj_buffer_create(0);

  //------------------------------------------------------------
  tj_buffer_appendString(src, "HEL$XLO!");

  tj_template_variables *vars = tj_template_variables_create();
  tj_template_variables_setFromString(vars, "X", "mushi");

  tj_template_variables_apply(vars, target, src);

  printf("Res: '%s'\n", tj_buffer_getAsString(target));

  if (strcmp(tj_buffer_getAsString(target), "HELmushiLO!")) {
    FAIL("Did not get expected accumulated string.");
  }


  //------------------------------------------------------------
  tj_template_variables_finalize(vars);
  vars = tj_template_variables_create();

  tj_buffer_reset(target);
  tj_buffer_reset(src);
  tj_buffer_appendString(src, "A $MAN, a $PLAN, a $CANAL, Panama!");

  tj_template_variables_setFromString(vars, "MAN", "man");
  tj_template_variables_setFromString(vars, "PLAN", "plan");
  tj_template_variables_setFromString(vars, "CANAL", "canal");

  tj_template_variables_apply(vars, target, src);

  printf("Res: '%s'\n", tj_buffer_getAsString(target));

  if (strcmp(tj_buffer_getAsString(target),
             "A man, a plan, a canal, Panama!")) {
    FAIL("Did not get expected accumulated string.");
  }

  //------------------------------------------------------------
  tj_template_variables_finalize(vars);
  vars = tj_template_variables_create();

  tj_buffer_reset(target);
  tj_buffer_reset(src);
  tj_buffer_appendString(src, "$MUSHI");

  tj_template_variables_setFromFile(vars, "MUSHI", "test/mushi");

  tj_template_variables_apply(vars, target, src);

  printf("Res: '%s'\n", tj_buffer_getAsString(target));

  if (strcmp(tj_buffer_getAsString(target), "MUSHI")) {
    FAIL("Did not get expected accumulated string.");
  }

  //------------------------------------------------------------
  tj_template_variables_finalize(vars);
  vars = tj_template_variables_create();

  tj_buffer_reset(target);
  tj_buffer_reset(src);
  tj_buffer_appendString(src, "$MUSHI");

  tj_template_variables_setFromString(vars, "X", "mushi");
  tj_template_variables_setFromFile(vars, "MUSHI", "test/mushi2");
  tj_template_variables_setRecurse(vars, "MUSHI", 1);

  tj_template_variables_apply(vars, target, src);

  printf("Res: '%s'\n", tj_buffer_getAsString(target));

  if (strcmp(tj_buffer_getAsString(target), "mushi mushi mushi")) {
    FAIL("Did not get expected accumulated string.");
  }

  //------------------------------------------------------------
  tj_buffer_finalize(target);
  tj_buffer_finalize(src);

  if (fail) {
    printf("** There were errors.\n");
    return -1;
  }

  tj_template_variables_finalize(vars);

  printf("Done.\n");

  return 0;

  // end main
}
