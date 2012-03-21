/* Copyright 2008 Mattias Norrby
 * 
 * This file is part of Test Dept..
 * 
 * Test Dept. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Test Dept. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Test Dept..  If not, see <http://www.gnu.org/licenses/>.
 */

#include <test-dept.h>
#include <sut.h>
#include <foo.h>
#include <stdio.h>
#include <stdlib.h>

void test_normal_fooify() {
  assert_not_equals(0, fooify(3));
}

void test_stringify() {
  char *h = stringify('h');
  assert_string_equals("h", h);
  free(h);
}

void *always_failing_malloc() {
  return NULL;
}

void test_stringify_cannot_malloc_returns_sane_result() {
  replace_function(&malloc, &always_failing_malloc);
  char *h = stringify('h');
  assert_string_equals("cannot_stringify", h);
}

int negative_foo(int value) {
  return -99 * value;
}

void test_broken_foo_makes_fooify_return_subzero() {
  replace_function(&foo, &negative_foo);
  assert_equals(-1, fooify(3));
}

void teardown() {
  restore_function(&malloc);
  restore_function(&foo);
}
