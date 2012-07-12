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

#include <tj_error.h>

int fail = 0;

#define FAIL(M, ...) {fail = 1; printf("FAIL: " M "\n", ##__VA_ARGS__);}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
tj_error *
terribleFunction(int a)
{
  if (a == 0)
    return tj_error_create(TJ_ERROR_API_MISUSE,
			   "This function was called improperly!");

  return TJ_ERROR_OK;

  // end terribleFunction
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
int
main(int argc, char *argv[])
{

  //--------------------------------------------
  tj_error *x;
  x = tj_error_create(TJ_ERROR_SOCKET, "Some bad thing happened.");
  tj_error_appendMessage(x, "The magic number is %d.", 12);
  printf("%s\n", tj_error_getMessage(x));
  tj_error_finalize(x);


  //--------------------------------------------
  if ((x = terribleFunction(0)) != TJ_ERROR_OK) {
    printf("Terrible 0---there was a problem: %s\n", tj_error_getMessage(x));
    tj_error_finalize(x);
  } else {
    printf("Terrible 0---no problem.\n");
  }


  //--------------------------------------------
  if ((x = terribleFunction(1)) != TJ_ERROR_OK) {
    printf("Terrible 1---there was a problem: %s\n", tj_error_getMessage(x));
    tj_error_finalize(x);
  } else {
    printf("Terrible 1---no problem.\n");
  }

  return 0;
  // end main
}
