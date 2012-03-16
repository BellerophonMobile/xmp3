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
#include <stdlib.h>
#include <string.h>

#include <tj_buffer.h>

int fail = 0;

#define FAIL(M, ...) {fail = 1; printf("FAIL: " M "\n", ##__VA_ARGS__);}

int
main(int argc, char *argv[])
{

  // Skipping the error checking...

  tj_buffer *buff1 = tj_buffer_create(0);

  tj_buffer_append(buff1, (tj_buffer_byte *) "HELLO", 5);

  if (tj_buffer_getAllocated(buff1) != 5)
    FAIL("Incorrect allocation!");

  if (tj_buffer_getUsed(buff1) != 5)
    FAIL("Incorrect used amount.");

  tj_buffer_append(buff1, (tj_buffer_byte *) "HELLO", 5);

  if (tj_buffer_getAllocated(buff1) != 10)
    FAIL("Incorrect increased allocation!");

  if (tj_buffer_getUsed(buff1) != 10)
    FAIL("Incorrect increased used amount.");

  tj_buffer_appendString(buff1, "HELLO");

  if (strcmp((char *) tj_buffer_getBytes(buff1), "HELLOHELLOHELLO"))
    FAIL("Accumulated string incorrect.");

  printf("Res: '%s'\n", (char *) tj_buffer_getBytes(buff1));

  if (tj_buffer_getAllocated(buff1) != 16)
    FAIL("Incorrect accumulated allocation!");

  if (tj_buffer_getUsed(buff1) != 16)
    FAIL("Incorrect accumulated used amount.");

  //--------------------------------------------
  tj_buffer_reset(buff1);

  if (tj_buffer_getAllocated(buff1) != 16)
    FAIL("Incorrect allocation.");

  if (tj_buffer_getUsed(buff1) != 0)
    FAIL("Incorrect used amount.");

  tj_buffer_appendAsString(buff1, "HELLO");
  tj_buffer_appendAsString(buff1, "HELLO");

  if (tj_buffer_getAllocated(buff1) != 16)
    FAIL("Incorrect allocation.");

  if (tj_buffer_getUsed(buff1) != 11)
    FAIL("Incorrect used amount.");

  if (strcmp(tj_buffer_getAsString(buff1), "HELLOHELLO"))
    FAIL("Accumulated reset string incorrect.");

  printf("Res: '%s'\n", tj_buffer_getAsString(buff1));

  //--------------------------------------------
  tj_buffer_reset(buff1);

  FILE *f;
  if ((f=fopen("test/mushi", "r")) == 0) {
    FAIL("Could not read test file test/mushi.");
  } else {
    tj_buffer_appendFileStream(buff1, f);
    fclose(f);
  }

  tj_buffer_appendString(buff1, "");
  printf("Res: '%s'\n", tj_buffer_getAsString(buff1));

  if (strcmp(tj_buffer_getAsString(buff1), "MUSHI"))
    FAIL("String read from file incorrect.");

  //--------------------------------------------
  tj_buffer_reset(buff1);

  if ((f=fopen("test/mushi", "r")) == 0) {
    FAIL("Could not read test file test/mushi.");
  } else {
    tj_buffer_appendFileStream(buff1, f);
    fclose(f);
  }
  if ((f=fopen("test/mushi", "r")) == 0) {
    FAIL("Could not read test file test/mushi.");
  } else {
    tj_buffer_appendFileStream(buff1, f);
    fclose(f);
  }
  if ((f=fopen("test/mushi", "r")) == 0) {
    FAIL("Could not read test file test/mushi.");
  } else {
    tj_buffer_appendFileStream(buff1, f);
    fclose(f);
  }
  if ((f=fopen("test/mushi", "r")) == 0) {
    FAIL("Could not read test file test/mushi.");
  } else {
    tj_buffer_appendFileStream(buff1, f);
    fclose(f);
  }

  tj_buffer_appendString(buff1, "");
  printf("Res: '%s'\n", tj_buffer_getAsString(buff1));

  if (strcmp(tj_buffer_getAsString(buff1), "MUSHIMUSHIMUSHIMUSHI"))
    FAIL("String read from file incorrect.");


  //--------------------------------------------
  tj_buffer_reset(buff1);

  if ((f=fopen(argv[0], "rb")) == 0) {
    FAIL("Could not read test file %s.", argv[0]);
  } else {
    fseek(f, 0L, SEEK_END);
    size_t flen = ftell(f);
    fseek(f, 0L, SEEK_SET);

    tj_buffer_appendFileStream(buff1, f);

    if (tj_buffer_getUsed(buff1) != flen)
      FAIL("Read %zu bytes from %s, expected %zu.",
           tj_buffer_getUsed(buff1), argv[0], flen);

    printf("Read %s; buffer[%zu/%zu]\n", argv[0],
           tj_buffer_getUsed(buff1), tj_buffer_getAllocated(buff1));

    fclose(f);
  }

  //--------------------------------------------
  tj_buffer_reset(buff1);

  if ((f=fopen(argv[0], "rb")) == 0) {
    FAIL("Could not read test file %s.", argv[0]);
  } else {
    fseek(f, 0L, SEEK_END);
    size_t flen = ftell(f);
    fseek(f, 0L, SEEK_SET);
    fclose(f);

    tj_buffer_appendFile(buff1, argv[0]);

    if (tj_buffer_getUsed(buff1) != flen)
      FAIL("Read %zu bytes from %s, expected %zu.",
           tj_buffer_getUsed(buff1), argv[0], flen);

    printf("Read %s; buffer[%zu/%zu]\n", argv[0],
           tj_buffer_getUsed(buff1), tj_buffer_getAllocated(buff1));
  }

  //--------------------------------------------
  tj_buffer_finalize(buff1);

  if (fail) {
    printf("** There were errors.\n");
    return -1;
  }

  printf("Done.\n");

  return 0;
  // end main
}
