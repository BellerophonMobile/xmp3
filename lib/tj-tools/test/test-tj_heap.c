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

#include <tj_heap.h>

int fail = 0;

#define FAIL(M, ...) {fail = 1; printf("FAIL: " M "\n", ##__VA_ARGS__);}

int intless(int a, int b) { return a  < b; }
int intfind(void *d, int k, char *v) { return (strcmp(v, d) == 0); }
TJ_HEAP_DECL(intheap, int, char *, intless);

int floatmore(float a, float b) { return a  > b; }
TJ_HEAP_DECL(floatheap, float, char *, floatmore);

int
main(int argc, char *argv[])
{

  // Skipping the error checking...

  //----------------------------------------------------
  intheap *heap = intheap_create(4);

  int k;
  char *v;

  intheap_add(heap, 923, "h");
  intheap_add(heap, 467, "d");
  intheap_add(heap, 23, "a1");
  intheap_add(heap, 500, "f1");
  intheap_add(heap, 23, "a2");
  intheap_add(heap, 234, "d");
  intheap_add(heap, 468, "e");
  intheap_add(heap, 900, "g");
  intheap_add(heap, 90, "c");
  intheap_add(heap, 500, "f2");
  intheap_add(heap, 80, "b");

  printf("Integer min heap.\n");
  if (intheap_peek(heap, &k, &v)) {
    printf("Peek %4d %6s\n", k, v);
  }

  while (intheap_pop(heap, &k, &v)) {
    printf("  %4d %6s\n", k, v);
  }
  printf("\n");

  intheap_finalize(heap);

  //----------------------------------------------------
  floatheap *fheap = floatheap_create(8);
  floatheap_add(fheap, 2.4, "2.4");
  floatheap_add(fheap, 0.3, "0.3");
  floatheap_add(fheap, 0.7, "0.7");
  floatheap_add(fheap, 7.8, "7.8");
  floatheap_add(fheap, 4.0, "4.0");

  float f;

  printf("Float max heap.\n");
  if (floatheap_peek(fheap, &f, &v)) {
    printf("Peek %4f %6s\n", f, v);
  }

  while (floatheap_pop(fheap, &f, &v)) {
    printf("  %4f %6s\n", f, v);
  }
  printf("\n");

  floatheap_finalize(fheap);

  //----------------------------------------------------
  heap = intheap_create(4);

  intheap_add(heap, 923, "d");
  intheap_add(heap, 467, "b");
  intheap_add(heap, 23, "a");
  intheap_add(heap, 500, "c");

  k = intheap_find(heap, &intfind, "a");
  if (k == -1) {
    printf("Could not find a.\n");
  } else {
    printf("Found a at %d.\n", k);
    intheap_remove(heap, k, &k, &v);
    printf("Removed %d:%s.\n", k, v);
  }

  k = intheap_find(heap, &intfind, "c");
  if (k == -1) {
    printf("Could not find a.\n");
  } else {
    printf("Found c at %d.\n", k);
    intheap_remove(heap, k, &k, &v);
    printf("Removed %d:%s.\n", k, v);
  }

  k = intheap_find(heap, &intfind, "x");
  if (k == -1) {
    printf("Could not find x.\n");
  } else {
    printf("Found x at %d.\n", k);
    intheap_remove(heap, k, &k, &v);
    printf("Removed %d:%s.\n", k, v);
  }

  while (intheap_pop(heap, &k, &v)) {
    printf("  %4d %6s\n", k, v);
  }
  printf("\n");

  intheap_finalize(heap);

  //----------------------------------------------------
  if (fail) {
    printf("** There were errors.\n");
    return -1;
  }

  printf("Done.\n");

  return 0;
  // end main
}
